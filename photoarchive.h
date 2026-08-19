#ifndef PHOTOARCHIVE_H
#define PHOTOARCHIVE_H

#include <QDateTime>
#include <QSize>
#include <QString>
#include <QVector>

struct PhotoRecord
{
    QString path;
    QString recordPath;
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
    bool exportRecord(const QString& sourcePath,
                      const QString& destinationDirectory,
                      QString* errorMessage = nullptr) const;

private:
    QString recordPathForImage(const QString& imagePath) const;
    QString m_directoryPath;
};

#endif // PHOTOARCHIVE_H
