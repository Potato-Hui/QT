#include "photoarchive.h"

#include <QCoreApplication>
#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>

#include <cassert>

namespace {

QString writeImage(const QString& directory,
                   const QString& fileName,
                   const QSize& size,
                   const QColor& color)
{
    const QString path = QDir(directory).filePath(fileName);
    QImage image(size, QImage::Format_RGB32);
    image.fill(color);
    assert(image.save(path, "JPG", 90));
    return path;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir sourceDirectory;
    QTemporaryDir exportDirectory;
    assert(sourceDirectory.isValid());
    assert(exportDirectory.isValid());

    const QString olderPath = writeImage(
        sourceDirectory.path(), "older.jpg", QSize(320, 240), Qt::red);
    const QString newerPath = writeImage(
        sourceDirectory.path(), "newer.png", QSize(640, 480), Qt::green);

    QFile olderFile(olderPath);
    QFile newerFile(newerPath);
    assert(olderFile.open(QIODevice::ReadWrite));
    assert(newerFile.open(QIODevice::ReadWrite));
    assert(olderFile.setFileTime(QDateTime::fromSecsSinceEpoch(1000),
                                 QFileDevice::FileModificationTime));
    assert(newerFile.setFileTime(QDateTime::fromSecsSinceEpoch(2000),
                                 QFileDevice::FileModificationTime));
    olderFile.close();
    newerFile.close();

    QFile note(QDir(sourceDirectory.path()).filePath("keep.txt"));
    assert(note.open(QIODevice::WriteOnly));
    assert(note.write("keep") == 4);
    note.close();

    PhotoArchive archive(sourceDirectory.path());
    const QVector<PhotoRecord> records = archive.records();
    assert(records.size() == 2);
    assert(records.at(0).path == newerPath);
    assert(records.at(0).imageSize == QSize(640, 480));
    assert(records.at(1).path == olderPath);
    assert(records.at(1).imageSize == QSize(320, 240));

    QString error;
    const QString exportedPath =
        QDir(exportDirectory.path()).filePath("copy.png");
    assert(archive.exportPhoto(newerPath, exportedPath, &error));
    assert(error.isEmpty());
    QFile source(newerPath);
    QFile exported(exportedPath);
    assert(source.open(QIODevice::ReadOnly));
    assert(exported.open(QIODevice::ReadOnly));
    assert(source.readAll() == exported.readAll());
    source.close();
    exported.close();

    assert(archive.remove(olderPath, &error));
    assert(error.isEmpty());
    assert(!QFileInfo::exists(olderPath));

    const PhotoClearResult clearResult = archive.clear();
    assert(clearResult.removed == 1);
    assert(clearResult.failed == 0);
    assert(archive.records().isEmpty());
    assert(QFileInfo::exists(note.fileName()));

    return 0;
}
