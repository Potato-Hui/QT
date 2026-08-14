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
    const QFileInfoList files = directory.entryInfoList(
        {QStringLiteral("*.jpg"),
         QStringLiteral("*.jpeg"),
         QStringLiteral("*.png")},
        QDir::Files | QDir::Readable,
        QDir::Time);

    QVector<PhotoRecord> result;
    result.reserve(files.size());
    for (const QFileInfo& file : files) {
        QImageReader reader(file.absoluteFilePath());
        PhotoRecord record;
        record.path = file.absoluteFilePath();
        record.fileName = file.fileName();
        record.modified = file.lastModified();
        record.bytes = file.size();
        record.imageSize = reader.size();
        result.push_back(record);
    }
    return result;
}

bool PhotoArchive::remove(const QString& path, QString* errorMessage) const
{
    setError(errorMessage, QString());
    if (!containsPath(path)) {
        setError(errorMessage, QStringLiteral("照片不在历史记录目录中"));
        return false;
    }
    if (!QFileInfo::exists(path)) {
        setError(errorMessage, QStringLiteral("照片文件不存在"));
        return false;
    }
    if (!QFile::remove(path)) {
        setError(errorMessage, QStringLiteral("无法删除照片：%1").arg(path));
        return false;
    }
    return true;
}

PhotoClearResult PhotoArchive::clear() const
{
    PhotoClearResult result;
    const QVector<PhotoRecord> currentRecords = records();
    for (const PhotoRecord& record : currentRecords) {
        if (QFile::remove(record.path)) {
            ++result.removed;
        } else {
            ++result.failed;
        }
    }
    return result;
}

bool PhotoArchive::exportPhoto(const QString& sourcePath,
                               const QString& destinationPath,
                               QString* errorMessage) const
{
    setError(errorMessage, QString());
    if (!containsPath(sourcePath) || !QFileInfo::exists(sourcePath)) {
        setError(errorMessage, QStringLiteral("源照片不存在或不在历史记录目录中"));
        return false;
    }

    const QString source = QFileInfo(sourcePath).absoluteFilePath();
    const QString destination = QFileInfo(destinationPath).absoluteFilePath();
    if (source == destination) {
        setError(errorMessage, QStringLiteral("导出位置不能与源照片相同"));
        return false;
    }
    if (QFileInfo::exists(destination)) {
        setError(errorMessage, QStringLiteral("导出目标已经存在"));
        return false;
    }
    if (!QFile::copy(source, destination)) {
        setError(errorMessage, QStringLiteral("无法导出照片到：%1").arg(destination));
        return false;
    }
    return true;
}

bool PhotoArchive::containsPath(const QString& path) const
{
    const QString parentPath = QFileInfo(path).absoluteDir().absolutePath();
    return parentPath == m_directoryPath;
}
