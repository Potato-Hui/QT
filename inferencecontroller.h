
#ifndef INFERENCECONTROLLER_H
#define INFERENCECONTROLLER_H

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QProcess>
#include <QString>
#include "inferenceprofile.h"
#include "snapshotprotocol.h"

class QTimer;
class GstVideoReceiver;

class InferenceController : public QObject
{
    Q_OBJECT

public:
    enum class State
    {
        Idle,
        Starting,
        Running,
        Stopping,
        Error
    };
    Q_ENUM(State)

    explicit InferenceController(
        InferenceProfile profile = InferenceProfile::DualModel,
        QObject *parent = nullptr);
    ~InferenceController() override;

    State state() const;

public slots:
    void startDetection();
    void stopDetection();
    void requestSnapshot(const SnapshotRequest& request);

signals:
    void frameReady(const QImage &frame);

    void stateChanged(
        InferenceController::State state,
        const QString &message);

    void metricsUpdated(
        double pipelineFps,
        double latencyMs);

    void logReceived(const QString &message);
    void snapshotReady(const SnapshotPackage& package);
    void snapshotFailed(const QString& requestId, const QString& message);

private slots:
    void readProcessOutput();
    void readProcessError();

    void handleProcessFinished(
        int exitCode,
        QProcess::ExitStatus exitStatus);

    void handleProcessError(
        QProcess::ProcessError error);

    void handleFirstFrame();
    void handleFrame(const QImage &frame);
    void handleStreamError(const QString &message);

    void handleStartupTimeout();
    void handleStopTimeout();
    void checkVideoTimeout();

private:
    void parseProtocolLine(const QByteArray &line);

    void setState(
        State state,
        const QString &message = QString());

    void enterError(const QString &message);
    void cleanupReceiver();
    void failPendingSnapshot(const QString& message);

    QProcess *m_process = nullptr;
    GstVideoReceiver *m_videoReceiver = nullptr;

    QTimer *m_startupTimer = nullptr;
    QTimer *m_stopTimer = nullptr;
    QTimer *m_videoWatchdog = nullptr;

    QElapsedTimer m_lastFrameTimer;

    State m_state = State::Idle;
    bool m_restartRequested = false;
    QString m_pendingSnapshotId;
    InferenceProfile m_profile = InferenceProfile::DualModel;
};


#endif // INFERENCECONTROLLER_H
