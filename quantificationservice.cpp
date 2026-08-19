#include "quantificationservice.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#ifdef INSULATOR_QUANTIFIER_AVAILABLE
#include <QtInsulatorQuantifier.hpp>
#include <exception>
#endif

namespace {

bool readJsonObject(const QString& path, QJsonObject* object, QString* errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *errorMessage = QStringLiteral("无法读取 JSON：%1").arg(path);
        return false;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        *errorMessage = QStringLiteral("JSON 格式无效：%1").arg(path);
        return false;
    }

    *object = document.object();
    return true;
}

bool integerValue(const QJsonValue& value, int* output)
{
    if (!value.isDouble()) {
        return false;
    }
    const double number = value.toDouble();
    const int integer = qRound(number);
    if (number != integer) {
        return false;
    }
    *output = integer;
    return true;
}

bool isValidMask(const QString& path, int width, int height)
{
    QImageReader reader(path);
    QImage mask = reader.read();
    if (mask.isNull() || mask.format() != QImage::Format_Grayscale8
        || mask.width() != width || mask.height() != height) {
        return false;
    }

    for (int y = 0; y < mask.height(); ++y) {
        const uchar* row = mask.constScanLine(y);
        for (int x = 0; x < mask.width(); ++x) {
            if (row[x] != 0 && row[x] != 255) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

QuantificationService::QuantificationService(const QString& recordsRoot,
                                             QObject* parent)
    : QObject(parent)
    , m_recordsRoot(QDir(recordsRoot).absolutePath())
{
}

void QuantificationService::process(const SnapshotPackage& package)
{
    QString errorMessage;
    if (!validatePackage(package, &errorMessage)) {
        emit failed(package, errorMessage);
        return;
    }

#ifdef INSULATOR_QUANTIFIER_AVAILABLE
    try {
        const QDir recordDir(package.recordDir);
        const QString resultPath = recordDir.filePath(QStringLiteral("result.json"));
        insulator::QtInsulatorQuantifier quantifier;
        quantifier.processFromFiles(
            recordDir.filePath(QStringLiteral("image.jpg")),
            recordDir.filePath(QStringLiteral("metadata.json")),
            QStringLiteral(INSULATOR_CALIBRATION_PATH), resultPath);

        QJsonObject result;
        if (!readJsonObject(resultPath, &result, &errorMessage)) {
            emit failed(package, QStringLiteral("量化结果未生成：%1").arg(errorMessage));
            return;
        }
        emit completed(package, result);
    } catch (const std::exception& error) {
        emit failed(package, QString::fromUtf8(error.what()));
    }
#else
#ifdef INSULATOR_DESKTOP_PREVIEW
    const QDir recordDir(package.recordDir);
    QJsonObject result;
    result.insert(QStringLiteral("basic_damage_index"), 0.0);
    result.insert(QStringLiteral("conservative_risk_index"), 0.0);
    result.insert(QStringLiteral("overall_level"), QStringLiteral("正常"));
    result.insert(QStringLiteral("recommendation"), QStringLiteral("预览版模拟量化结果"));
    result.insert(QStringLiteral("highest_risk_disc_id"), 1);
    result.insert(QStringLiteral("disc_count"), 1);
    result.insert(QStringLiteral("defect_instance_count"), 0);
    QFile resultFile(recordDir.filePath(QStringLiteral("result.json")));
    if (!resultFile.open(QIODevice::WriteOnly)
        || resultFile.write(QJsonDocument(result).toJson(QJsonDocument::Indented)) < 0) {
        emit failed(package, QStringLiteral("无法保存预览量化结果"));
        return;
    }
    emit completed(package, result);
#else
    emit failed(package, QStringLiteral("当前构建未链接量化模块"));
#endif
#endif
}

bool QuantificationService::validatePackage(const SnapshotPackage& package,
                                            QString* errorMessage) const
{
    const QFileInfo recordInfo(package.recordDir);
    if (package.requestId.isEmpty() || !recordInfo.isDir()
        || recordInfo.absoluteDir().absolutePath() != m_recordsRoot) {
        *errorMessage = QStringLiteral("快照目录不在记录根目录中");
        return false;
    }

    const QDir recordDir(recordInfo.absoluteFilePath());
    const QString imagePath = recordDir.filePath(QStringLiteral("image.jpg"));
    const QString metadataPath = recordDir.filePath(QStringLiteral("metadata.json"));
    QImageReader imageReader(imagePath);
    const QSize imageSize = imageReader.size();
    if (imageSize.isEmpty()) {
        *errorMessage = QStringLiteral("快照原图无法读取");
        return false;
    }

    QJsonObject metadata;
    if (!readJsonObject(metadataPath, &metadata, errorMessage)) {
        return false;
    }

    int imageWidth = 0;
    int imageHeight = 0;
    if (!integerValue(metadata.value(QStringLiteral("image_width")), &imageWidth)
        || !integerValue(metadata.value(QStringLiteral("image_height")), &imageHeight)
        || imageWidth != imageSize.width() || imageHeight != imageSize.height()
        || !metadata.value(QStringLiteral("instances")).isArray()) {
        *errorMessage = QStringLiteral("metadata 图像尺寸或实例列表无效");
        return false;
    }

    const QStringList classes = {
        QStringLiteral("insulator"), QStringLiteral("crack"),
        QStringLiteral("pollution"), QStringLiteral("flashover"),
        QStringLiteral("broken")};
    const QJsonArray instances = metadata.value(QStringLiteral("instances")).toArray();
    for (const QJsonValue& value : instances) {
        if (!value.isObject()) {
            *errorMessage = QStringLiteral("metadata 包含无效实例");
            return false;
        }
        const QJsonObject instance = value.toObject();
        int instanceId = 0;
        int classId = -1;
        const double confidence = instance.value(QStringLiteral("confidence")).toDouble(-1.0);
        if (!integerValue(instance.value(QStringLiteral("instance_id")), &instanceId)
            || !integerValue(instance.value(QStringLiteral("class_id")), &classId)
            || instanceId < 1
            || classId < 0 || classId >= classes.size()
            || instance.value(QStringLiteral("class_name")).toString() != classes.at(classId)
            || confidence < 0.0 || confidence > 1.0) {
            *errorMessage = QStringLiteral("metadata 类别或置信度无效");
            return false;
        }

        const QJsonArray bbox = instance.value(QStringLiteral("bbox_xyxy")).toArray();
        int x1 = 0;
        int y1 = 0;
        int x2 = 0;
        int y2 = 0;
        if (bbox.size() != 4 || !integerValue(bbox.at(0), &x1)
            || !integerValue(bbox.at(1), &y1) || !integerValue(bbox.at(2), &x2)
            || !integerValue(bbox.at(3), &y2) || x1 < 0 || y1 < 0
            || x2 <= x1 || y2 <= y1 || x2 > imageWidth || y2 > imageHeight) {
            *errorMessage = QStringLiteral("metadata bbox 无效");
            return false;
        }

        const QString maskFile = instance.value(QStringLiteral("mask_file")).toString();
        if (QFileInfo(maskFile).isAbsolute() || !maskFile.startsWith(QStringLiteral("masks/"))
            || !maskFile.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)
            || maskFile.contains(QStringLiteral(".."))
            || !isValidMask(recordDir.filePath(maskFile), x2 - x1, y2 - y1)) {
            *errorMessage = QStringLiteral("metadata mask 不符合接口要求");
            return false;
        }
    }
    return true;
}
