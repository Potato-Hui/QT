#ifndef QUANTIFICATIONSERVICE_H
#define QUANTIFICATIONSERVICE_H

#include "snapshotprotocol.h"

#include <QObject>

class QJsonObject;

class QuantificationService : public QObject
{
    Q_OBJECT

public:
    explicit QuantificationService(const QString& recordsRoot,
                                   QObject* parent = nullptr);

public slots:
    void process(const SnapshotPackage& package);

signals:
    void completed(const SnapshotPackage& package, const QJsonObject& result);
    void failed(const SnapshotPackage& package, const QString& message);

private:
    bool validatePackage(const SnapshotPackage& package,
                         QString* errorMessage) const;

    QString m_recordsRoot;
};

#endif // QUANTIFICATIONSERVICE_H
