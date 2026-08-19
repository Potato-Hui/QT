#ifndef SNAPSHOTPROTOCOL_H
#define SNAPSHOTPROTOCOL_H

#include <QByteArray>
#include <QMetaType>
#include <QString>

struct SnapshotRequest
{
    QString requestId;
    QString outputDir;
};

struct SnapshotPackage
{
    QString requestId;
    QString recordDir;
};

Q_DECLARE_METATYPE(SnapshotRequest)
Q_DECLARE_METATYPE(SnapshotPackage)

QByteArray snapshotRequestLine(const SnapshotRequest& request);

#endif // SNAPSHOTPROTOCOL_H
