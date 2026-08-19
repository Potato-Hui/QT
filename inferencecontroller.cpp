#include "inferencecontroller.h"
#include "gstvideoreceiver.h"
#include "inferencelaunchspec.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QByteArray>
DetectionMode m_detectionMode =DetectionMode::StandardCamera;

InferenceController::InferenceController(
    InferenceProfile profile,
    QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_videoReceiver(new GstVideoReceiver(this))
    , m_startupTimer(new QTimer(this))
    , m_stopTimer(new QTimer(this))
    , m_videoWatchdog(new QTimer(this))
    , m_profile(profile)
{
    m_startupTimer->setSingleShot(true);
    m_startupTimer->setInterval(30000);

    m_stopTimer->setSingleShot(true);
    m_stopTimer->setInterval(5000);

    m_videoWatchdog->setInterval(250);

    connect(
        m_process,
        &QProcess::readyReadStandardOutput,
        this,
        &InferenceController::readProcessOutput);

    connect(
        m_process,
        &QProcess::readyReadStandardError,
        this,
        &InferenceController::readProcessError);

    connect(
        m_process,
        QOverload<int, QProcess::ExitStatus>::of(
            &QProcess::finished),
        this,
        &InferenceController::handleProcessFinished);

    connect(
        m_process,
        &QProcess::errorOccurred,
        this,
        &InferenceController::handleProcessError);

    connect(
        m_process,
        &QProcess::started,
        this,
        [this]() {
            emit logReceived(QStringLiteral("RKNN推理程序已启动，等待就绪"));
        });

    connect(
        m_videoReceiver,
        &GstVideoReceiver::frameReady,
        this,
        &InferenceController::handleFrame,
        Qt::AutoConnection);

    connect(
        m_videoReceiver,
        &GstVideoReceiver::firstFrameReceived,
        this,
        &InferenceController::handleFirstFrame,
        Qt::QueuedConnection);

    connect(
        m_videoReceiver,
        &GstVideoReceiver::streamError,
        this,
        &InferenceController::handleStreamError,
        Qt::QueuedConnection);

    connect(
        m_startupTimer,
        &QTimer::timeout,
        this,
        &InferenceController::handleStartupTimeout);

    connect(
        m_stopTimer,
        &QTimer::timeout,
        this,
        &InferenceController::handleStopTimeout);

    connect(
        m_videoWatchdog,
        &QTimer::timeout,
        this,
        &InferenceController::checkVideoTimeout);
}
InferenceController::~InferenceController()
{
    m_startupTimer->stop();
    m_stopTimer->stop();
    m_videoWatchdog->stop();

    m_videoReceiver->stop();

    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();

        if (!m_process->waitForFinished(1000)) {
            m_process->kill();
            m_process->waitForFinished(1000);
        }
    }
}
void InferenceController::startDetection()
{
    /*
     * 当前不是空闲或错误状态时，不允许重复启动。
     */
    if (m_state != State::Idle &&
        m_state != State::Error) {
        return;
    }

    /*
     * 如果上一次的 RKNN 进程还没有退出，
     * 先清理旧进程。
     */
    if (m_process->state() !=
        QProcess::NotRunning) {

        if (m_state == State::Error) {
            m_restartRequested = true;

            emit logReceived(
                QStringLiteral(
                    "正在清理上一次推理进程，清理完成后重新启动"));

            m_process->terminate();
            m_stopTimer->start();
        }

        return;
    }

    m_restartRequested = false;

    /*
     * 停止并释放上一次的视频管线。
     */
    cleanupReceiver();

    /*
     * 热像仪模式：
     * 不启动 RKNN，直接连接 RTSP 视频流。
     */
    if (m_detectionMode ==
        DetectionMode::ThermalCamera) {

        setState(
            State::Starting,
            QStringLiteral("正在连接热像仪"));

        /*
         * 如果在规定时间内没有收到第一帧，
         * handleStartupTimeout() 会报告启动超时。
         */
        m_startupTimer->start();

        const bool started =
            m_videoReceiver->start(
                GstVideoReceiver::VideoSource::
                    ThermalRtsp);

        if (!started) {
            m_startupTimer->stop();

            enterError(
                QStringLiteral(
                    "热像仪视频管线启动失败"));

            return;
        }

        /*
         * 管线启动不代表已经收到视频。
         * 收到第一帧后，handleFirstFrame()
         * 才会把状态改成 Running。
         */
        return;
    }

    /*
     * 标准摄像头模式：
     * 继续使用原来的 RKNN 推理程序。
     */
    const LaunchSpecResult launch =
        buildInferenceLaunchSpec(
            m_profile,
            QCoreApplication::applicationDirPath());

    if (!launch.ok) {
        enterError(launch.error);
        return;
    }

    m_process->setWorkingDirectory(
        launch.spec.workingDirectory);

    m_process->setProgram(
        launch.spec.program);

    m_process->setArguments(
        launch.spec.arguments);

    m_process->setProcessChannelMode(
        QProcess::SeparateChannels);

    setState(
        State::Starting,
        QStringLiteral("正在启动推理程序"));

    m_startupTimer->start();
    m_process->start();
}
void InferenceController::readProcessOutput()
{
    while (m_process->canReadLine()) {
        const QByteArray line =
            m_process->readLine().trimmed();

        if (!line.isEmpty()) {
            emit logReceived(QString::fromUtf8(line));
            parseProtocolLine(line);
        }
    }
}

void InferenceController::readProcessError()
{
    const QString text = QString::fromUtf8(
        m_process->readAllStandardError()).trimmed();

    if (!text.isEmpty()) {
        emit logReceived(text);
    }
}

void InferenceController::parseProtocolLine(
    const QByteArray &line)
{
    QByteArray prefix;
    QByteArray jsonData;

    if (line.startsWith("@status ")) {
        prefix = "@status";
        jsonData = line.mid(8);
    } else if (line.startsWith("@metrics ")) {
        prefix = "@metrics";
        jsonData = line.mid(9);
    } else if (line.startsWith("@error ")) {
        prefix = "@error";
        jsonData = line.mid(7);
    } else {
        return;
    }

    QJsonParseError parseError;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            jsonData,
            &parseError);

    if (parseError.error != QJsonParseError::NoError ||
        !document.isObject()) {
        emit logReceived(
            QStringLiteral("无效状态消息：%1")
                .arg(QString::fromUtf8(line)));
        return;
    }

    const QJsonObject object = document.object();

    if (prefix == "@status") {
        const QString state =
            object.value(QStringLiteral("state"))
                .toString();

        if (state == QStringLiteral("ready") &&
            m_state == State::Starting) {

            if (!m_videoReceiver->start(
                    GstVideoReceiver::VideoSource::
                        RknnTcpJpeg)) {

                enterError(
                    QStringLiteral(
                        "TCP 视频接收启动失败"));
            }
            if (!m_videoReceiver->start(
                    GstVideoReceiver::VideoSource::
                        ThermalRtsp)) {

                enterError(
                    QStringLiteral(
                        "热像仪视频管线启动失败"));
            }
        }

        return;
    }

    if (prefix == "@metrics") {
        const double pipelineFps =
            object.value(
                QStringLiteral("pipeline_fps"))
                .toDouble();

        const double latencyMs =
            object.value(
                QStringLiteral("latency_ms"))
                .toDouble();

        emit metricsUpdated(
            pipelineFps,
            latencyMs);

        return;
    }

    if (prefix == "@error") {
        const QString message =
            object.value(QStringLiteral("message"))
                .toString(
                    QStringLiteral("推理程序发生错误"));

        enterError(message);
    }
}
void InferenceController::handleFrame(
    const QImage &frame)
{
    m_lastFrameTimer.restart();
    emit frameReady(frame);
}

void InferenceController::handleFirstFrame()
{
    if (m_state != State::Starting) {
        return;
    }

    m_startupTimer->stop();
    m_lastFrameTimer.start();
    m_videoWatchdog->start();

    setState(
        State::Running,
        QStringLiteral("正在检测"));
}
void InferenceController::stopDetection()
{
    if (m_state == State::Idle ||
        m_state == State::Stopping) {
        return;
    }

    m_startupTimer->stop();
    m_videoWatchdog->stop();

    setState(
        State::Stopping,
        QStringLiteral("正在停止"));

    if (m_process->state() ==
        QProcess::NotRunning) {

        cleanupReceiver();

        setState(
            State::Idle,
            QStringLiteral("等待开始"));

        return;
    }

    m_process->terminate();
    m_stopTimer->start();
}

void InferenceController::handleStopTimeout()
{
    if (m_process->state() !=
        QProcess::NotRunning) {
        m_process->kill();
    }
}
void InferenceController::handleProcessFinished(
    int exitCode,
    QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)

    m_startupTimer->stop();
    m_stopTimer->stop();
    m_videoWatchdog->stop();

    cleanupReceiver();

    if (m_state == State::Stopping) {
        setState(
            State::Idle,
            QStringLiteral("等待开始"));
        return;
    }

    if (m_state == State::Error) {
        if (m_restartRequested) {
            m_restartRequested = false;
            QTimer::singleShot(
                0,
                this,
                &InferenceController::startDetection);
        }
        return;
    }

    const QString message =
        exitStatus == QProcess::CrashExit
        ? QStringLiteral("推理程序异常退出")
        : QStringLiteral("推理程序已退出");

    setState(State::Error, message);
}

void InferenceController::handleProcessError(
    QProcess::ProcessError error)
{
    if (m_state == State::Stopping ||
        (m_state == State::Error &&
         m_restartRequested)) {
        return;
    }
    switch (error) {
    case QProcess::FailedToStart:
        enterError(
            QStringLiteral("无法启动推理程序"));
        break;

    case QProcess::Crashed:
        enterError(
            QStringLiteral("推理程序崩溃"));
        break;

    default:
        enterError(
            m_process->errorString());
        break;
    }
}
void InferenceController::handleStreamError(
    const QString &message)
{
    if (m_state == State::Stopping ||
        m_state == State::Idle) {
        return;
    }

    enterError(
        QStringLiteral("视频连接异常：%1")
            .arg(message));
}
void InferenceController::checkVideoTimeout()
{
    if (m_state != State::Running ||
        !m_lastFrameTimer.isValid()) {
        return;
    }

    if (m_lastFrameTimer.elapsed() > 3000) {
        enterError(
            QStringLiteral("超过 3 秒未收到视频帧"));
    }
}
void InferenceController::handleStartupTimeout()
{
    if (m_state == State::Starting) {
        enterError(
            QStringLiteral("推理程序启动超时"));
    }
}
void InferenceController::enterError(
    const QString &message)
{
    m_restartRequested = false;
    m_startupTimer->stop();
    m_videoWatchdog->stop();

    cleanupReceiver();
    setState(State::Error, message);

    if (m_process->state() !=
        QProcess::NotRunning) {
        m_process->terminate();
        m_stopTimer->start();
    }
}
void InferenceController::setState(
    State state,
    const QString &message)
{
    m_state = state;
    emit stateChanged(state, message);
}
InferenceController::State
InferenceController::state() const
{
    return m_state;
}
void InferenceController::cleanupReceiver()
{
    if (m_videoReceiver != nullptr) {
        m_videoReceiver->stop();
    }
}
void InferenceController::setDetectionMode(
    DetectionMode mode)
{
    if (m_state != State::Idle &&
        m_state != State::Error) {
        return;
    }

    m_detectionMode = mode;

    emit logReceived(
        mode == DetectionMode::ThermalCamera
            ? QStringLiteral("已选择热像仪模式")
            : QStringLiteral("已选择普通摄像头模式"));
}
