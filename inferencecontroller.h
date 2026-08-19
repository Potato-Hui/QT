
#ifndef INFERENCECONTROLLER_H
#define INFERENCECONTROLLER_H

#include "cameramode.h"
#include "inferenceprofile.h"

#include <QElapsedTimer>
#include <QImage>
#include <QObject>
#include <QProcess>
#include <QString>

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
    CameraMode selectedMode() const;
    CameraMode activeMode() const;

public slots:
    void startDetection();
    void stopDetection();
    void selectCameraMode(CameraMode mode);

signals:
    void frameReady(const QImage &frame);

    void stateChanged(
        InferenceController::State state,
        const QString &message);

    void metricsUpdated(
        double pipelineFps,
        double latencyMs);

    void logReceived(const QString &message);

    void selectedModeChanged(
        CameraMode mode,
        const QString &message);

private slots:
    void readProcessOutput();
    void readProcessError();

    void handleProcessFinished(
        int exitCode,
        QProcess::ExitStatus exitStatus);

    void handleProcessError(
        QProcess::ProcessError error);

    void handleNetworkFinished(
        int exitCode,
        QProcess::ExitStatus exitStatus);

    void handleNetworkError(
        QProcess::ProcessError error);

    void handleFirstFrame();
    void handleFrame(const QImage &frame);
    void handleStreamError(const QString &message);

    void handleStartupTimeout();
    void handleStopTimeout();
    void checkVideoTimeout();

private:
    enum class CleanupOutcome
    {
        Idle,
        StartSelected,
        Error
    };

    struct ThermalCameraConfig
    {
        QString interfaceName;
        QString address;
        QString netmask;
        QString url;
        int latency = 0;
    };

    void parseProtocolLine(const QByteArray &line);

    void startSelectedMode();
    void startVisibleLight();
    void startThermal();
    bool loadThermalConfig(
        ThermalCameraConfig &config,
        QString &errorMessage) const;
    QString findIfconfig() const;
    QString networkFailureMessage(
        const QString &standardError) const;

    void beginCleanup(
        CleanupOutcome outcome,
        const QString &message);
    void completeCleanup();
    void maybeCompleteCleanup();
    bool hasManagedResources() const;
    QString selectedModeMessage(CameraMode mode) const;

    void setState(
        State state,
        const QString &message = QString());

    void enterError(const QString &message);
    void cleanupReceiver();

    QProcess *m_process = nullptr;
    QProcess *m_networkProcess = nullptr;
    GstVideoReceiver *m_videoReceiver = nullptr;

    QTimer *m_startupTimer = nullptr;
    QTimer *m_stopTimer = nullptr;
    QTimer *m_videoWatchdog = nullptr;

    QElapsedTimer m_lastFrameTimer;

    State m_state = State::Idle;
    CameraMode m_selectedMode = CameraMode::VisibleLight;
    CameraMode m_activeMode = CameraMode::None;
    CleanupOutcome m_cleanupOutcome = CleanupOutcome::Idle;
    bool m_cleanupInProgress = false;
    QString m_cleanupMessage;
    ThermalCameraConfig m_thermalConfig;
    InferenceProfile m_profile = InferenceProfile::DualModel;
};


#endif // INFERENCECONTROLLER_H
