#include "photoarchive.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>

namespace {

void setError(QString* destination, const QString& message)
{
    if (destination != nullptr) {
        *destination = message;
    }
}

} // namespace

qint64 directorySize(const QDir& directory)
{
    qint64 total = 0;
    const QFileInfoList entries = directory.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
    for (const QFileInfo& entry : entries) {
        total += entry.isDir() ? directorySize(QDir(entry.absoluteFilePath())) : entry.size();
    }
    return total;
}

bool copyDirectory(const QDir& source, const QDir& destination)
{
    if (!QDir().mkpath(destination.absolutePath())) {
        return false;
    }
    const QFileInfoList entries = source.entryInfoList(
        QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
    for (const QFileInfo& entry : entries) {
        const QString target = destination.filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyDirectory(QDir(entry.absoluteFilePath()), QDir(target))) {
                return false;
            }
        } else if (!QFile::copy(entry.absoluteFilePath(), target)) {
            return false;
        }
    }
    return true;
}

PhotoArchive::PhotoArchive(const QString& directoryPath)
    : m_directoryPath(QDir(directoryPath).absolutePath())
{
}

QString PhotoArchive::directoryPath() const
{
    return m_directoryPath;
}

QVector<PhotoRecord> PhotoArchive::records() const
{
    const QDir directory(m_directoryPath);
    const QFileInfoList directories = directory.entryInfoList(
        QDir::Dirs | QDir::Readable | QDir::NoDotAndDotDot,
        QDir::Time);

    QVector<PhotoRecord> result;
    result.reserve(directories.size());
    for (const QFileInfo& directoryInfo : directories) {
        const QDir recordDirectory(directoryInfo.absoluteFilePath());
        const QString imagePath = recordDirectory.filePath(QStringLiteral("image.jpg"));
        const QString resultPath = recordDirectory.filePath(QStringLiteral("result.json"));
        if (!QFileInfo(imagePath).isReadable() || !QFileInfo(resultPath).isReadable()) {
            continue;
        }
        QImageReader reader(imagePath);
        PhotoRecord record;
        record.path = imagePath;
        record.fileName = directoryInfo.fileName();
        record.modified = directoryInfo.lastModified();
        record.bytes = directorySize(recordDirectory);
        record.imageSize = reader.size();
        result.push_back(record);
    }
    return result;
}

bool PhotoArchive::remove(const QString& path, QString* errorMessage) const
{
    setError(errorMessage, QString());
    const QString recordPath = recordPathForImage(path);
    if (recordPath.isEmpty()) {
        setError(errorMessage, QStringLiteral("照片不在历史记录目录中"));
        return false;
    }
    if (!QFileInfo::exists(recordPath)) {
        setError(errorMessage, QStringLiteral("记录目录不存在"));
        return false;
    }
    if (!QDir(recordPath).removeRecursively()) {
        setError(errorMessage, QStringLiteral("无法删除记录：%1").arg(recordPath));
        return false;
    }
    return true;
}

PhotoClearResult PhotoArchive::clear() const
{
    PhotoClearResult result;
    const QVector<PhotoRecord> currentRecords = records();
    for (const PhotoRecord& record : currentRecords) {
        if (QDir(recordPathForImage(record.path)).removeRecursively()) {
            ++result.removed;
        } else {
            ++result.failed;
        }
    }
    return result;
}

bool PhotoArchive::exportRecord(const QString& sourcePath,
                                const QString& destinationDirectory,
                                QString* errorMessage) const
{
    setError(errorMessage, QString());
    const QString source = recordPathForImage(sourcePath);
    if (source.isEmpty() || !QFileInfo::exists(source)) {
        setError(errorMessage, QStringLiteral("源记录不存在或不在历史记录目录中"));
        return false;
    }

    const QString destination = QDir(destinationDirectory).filePath(QFileInfo(source).fileName());
    if (QFileInfo::exists(destination)) {
        setError(errorMessage, QStringLiteral("导出目标已经存在"));
        return false;
    }
    if (!copyDirectory(QDir(source), QDir(destination))) {
        QDir(destination).removeRecursively();
        setError(errorMessage, QStringLiteral("无法导出记录到：%1").arg(destination));
        return false;
    }
    return true;
}

QString PhotoArchive::recordPathForImage(const QString& imagePath) const
{
    const QFileInfo imageInfo(imagePath);
    if (imageInfo.fileName() != QStringLiteral("image.jpg")) {
        return QString();
    }
    const QString recordPath = imageInfo.absoluteDir().absolutePath();
    if (QFileInfo(recordPath).absoluteDir().absolutePath() != m_directoryPath) {
        return QString();
    }
    return recordPath;
}
