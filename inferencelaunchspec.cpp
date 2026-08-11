#include "inferencelaunchspec.h"
#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace {
QString resolvePath(const QString& root, const QString& value)
{
    return QFileInfo(value).isAbsolute()
        ? QDir::cleanPath(value)
        : QDir(root).absoluteFilePath(value);
}

LaunchSpecResult validate(const InferenceLaunchSpec& spec)
{
    LaunchSpecResult result;
    result.spec = spec;
    const QFileInfo directory(spec.workingDirectory);
    const QFileInfo program(spec.program);
    if (!directory.isDir()) result.error = QStringLiteral("RKNN目录不存在：%1").arg(spec.workingDirectory);
    else if (!program.isFile()
#ifdef Q_OS_WIN
             || !program.isReadable()
#else
             || !program.isExecutable()
#endif
             ) result.error = QStringLiteral("RKNN程序不存在或不可执行：%1").arg(spec.program);
    else if (spec.arguments.isEmpty() || !QFileInfo(spec.arguments.first()).isReadable()) result.error = QStringLiteral("RKNN模型不存在或不可读：%1").arg(spec.arguments.value(0));
    else result.ok = true;
    return result;
}
}

LaunchSpecResult buildInferenceLaunchSpec(
    InferenceProfile profile, const QString& applicationDirectory)
{
    if (profile == InferenceProfile::SingleModel) {
        const QString iniPath = QDir(applicationDirectory).filePath("single_model.ini");
        if (!QFileInfo::exists(iniPath)) return {false, {}, QStringLiteral("找不到配置文件：%1").arg(iniPath)};
        QSettings settings(iniPath, QSettings::IniFormat);
        settings.beginGroup("Inference");
        const QString rootValue = settings.value("rknn_dir").toString().trimmed();
        const QString programValue = settings.value("program").toString().trimmed();
        const QString modelValue = settings.value("model").toString().trimmed();
        settings.endGroup();
        if (rootValue.isEmpty() || programValue.isEmpty() || modelValue.isEmpty())
            return {false, {}, QStringLiteral("single_model.ini 缺少 rknn_dir、program 或 model")};
        const QString root = QDir(rootValue).absolutePath();
        return validate({root, resolvePath(root, programValue),
                         {resolvePath(root, modelValue), "/dev/video41", "12"}});
    }

    QString root = qEnvironmentVariable("INSULATOR_RKNN_DIR");
    if (root.isEmpty()) root = applicationDirectory;
    root = QDir(root).absolutePath();
    InferenceLaunchSpec spec;
    spec.workingDirectory = root;
    spec.program = QDir(root).filePath("insulator_pipeline");
    spec.arguments = QStringList{
        QDir(root).filePath("model/RK3588/mixdet_fp16.rknn"),
        QDir(root).filePath("model/RK3588/v8n-seg.rknn"),
        "/dev/video41", "mjpg"};
    return validate(spec);
}
