#include "photoarchive.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>

#include <algorithm>

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
        if (!QFileInfo(imagePath).isReadable()) {
            continue;
        }
        QImageReader reader(imagePath);
        PhotoRecord record;
        record.path = imagePath;
        record.recordPath = directoryInfo.absoluteFilePath();
        record.fileName = directoryInfo.fileName();
        record.modified = directoryInfo.lastModified();
        record.bytes = directorySize(recordDirectory);
        record.imageSize = reader.size();
        result.push_back(record);
    }

    const QFileInfoList legacyFiles = directory.entryInfoList(
        {QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.png")},
        QDir::Files | QDir::Readable,
        QDir::Time);
    for (const QFileInfo& file : legacyFiles) {
        QImageReader reader(file.absoluteFilePath());
        PhotoRecord record;
        record.path = file.absoluteFilePath();
        record.recordPath = file.absoluteFilePath();
        record.fileName = file.fileName();
        record.modified = file.lastModified();
        record.bytes = file.size();
        record.imageSize = reader.size();
        result.push_back(record);
    }

#ifdef Q_OS_WIN
    const QDir previousDirectory(QFileInfo(m_directoryPath).absoluteDir());
    const QFileInfoList previousFiles = previousDirectory.entryInfoList(
        {QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.png")},
        QDir::Files | QDir::Readable,
        QDir::Time);
    for (const QFileInfo& file : previousFiles) {
        QImageReader reader(file.absoluteFilePath());
        PhotoRecord record;
        record.path = file.absoluteFilePath();
        record.recordPath = file.absoluteFilePath();
        record.fileName = file.fileName();
        record.modified = file.lastModified();
        record.bytes = file.size();
        record.imageSize = reader.size();
        result.push_back(record);
    }
#endif

    std::sort(result.begin(), result.end(), [](const PhotoRecord& left,
                                                const PhotoRecord& right) {
        return left.modified > right.modified;
    });
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
    const bool removed = QFileInfo(recordPath).isDir()
        ? QDir(recordPath).removeRecursively()
        : QFile::remove(recordPath);
    if (!removed) {
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
        const QString recordPath = recordPathForImage(record.path);
        const bool removed = QFileInfo(recordPath).isDir()
            ? QDir(recordPath).removeRecursively()
            : QFile::remove(recordPath);
        if (removed) {
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
    const bool copied = QFileInfo(source).isDir()
        ? copyDirectory(QDir(source), QDir(destination))
        : QFile::copy(source, destination);
    if (!copied) {
        if (QFileInfo(destination).isDir()) {
            QDir(destination).removeRecursively();
        } else {
            QFile::remove(destination);
        }
        setError(errorMessage, QStringLiteral("无法导出记录到：%1").arg(destination));
        return false;
    }
    return true;
}

QString PhotoArchive::recordPathForImage(const QString& imagePath) const
{
    const QFileInfo imageInfo(imagePath);
    const QString parentPath = imageInfo.absoluteDir().absolutePath();
    if (imageInfo.fileName() != QStringLiteral("image.jpg")) {
        if (parentPath == m_directoryPath) {
            return imageInfo.absoluteFilePath();
        }
#ifdef Q_OS_WIN
        if (parentPath == QFileInfo(m_directoryPath).absoluteDir().absolutePath()) {
            return imageInfo.absoluteFilePath();
        }
#endif
        return QString();
    }
    if (QFileInfo(parentPath).absoluteDir().absolutePath() == m_directoryPath) {
        return parentPath;
    }
    return parentPath == m_directoryPath ? imageInfo.absoluteFilePath() : QString();
}
