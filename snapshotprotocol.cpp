#include "snapshotprotocol.h"

#include <QJsonDocument>
#include <QJsonObject>

QByteArray snapshotRequestLine(const SnapshotRequest& request)
{
    QJsonObject object;
    object.insert(QStringLiteral("request_id"), request.requestId);
    object.insert(QStringLiteral("output_dir"), request.outputDir);
    return QByteArrayLiteral("@snapshot ")
        + QJsonDocument(object).toJson(QJsonDocument::Compact)
        + '\n';
}
