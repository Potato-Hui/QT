#ifndef PHOTOARCHIVE_H
#define PHOTOARCHIVE_H

#include <QDateTime>
#include <QSize>
#include <QString>
#include <QVector>

struct PhotoRecord
{
    QString path;
    QString fileName;
    QDateTime modified;
    qint64 bytes = 0;
    QSize imageSize;
};

struct PhotoClearResult
{
    int removed = 0;
    int failed = 0;
};

class PhotoArchive
{
public:
    explicit PhotoArchive(const QString& directoryPath);

    QString directoryPath() const;
    QVector<PhotoRecord> records() const;
    bool remove(const QString& path, QString* errorMessage = nullptr) const;
    PhotoClearResult clear() const;
    bool exportPhoto(const QString& sourcePath,
                     const QString& destinationPath,
                     QString* errorMessage = nullptr) const;

private:
    bool containsPath(const QString& path) const;
    QString m_directoryPath;
};

#endif // PHOTOARCHIVE_H
