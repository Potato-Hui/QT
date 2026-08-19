#include "inferencecontroller.h"

#include "gstvideoreceiver.h"
#include "inferencelaunchspec.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>

namespace {

QString redactRtspUrls(QString message)
{
    static const QRegularExpression expression(
        QStringLiteral("rtsps?://[^\\s\\\"']+"),
        QRegularExpression::CaseInsensitiveOption);
    message.replace(expression, QStringLiteral("rtsp://<已隐藏>"));
    return message;
}

} // namespace

InferenceController::InferenceController(
    InferenceProfile profile,
    QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_networkProcess(new QProcess(this))
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

    connect(m_process,
            &QProcess::readyReadStandardOutput,
            this,
            &InferenceController::readProcessOutput);
    connect(m_process,
            &QProcess::readyReadStandardError,
            this,
            &InferenceController::readProcessError);
    connect(m_process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &InferenceController::handleProcessFinished);
    connect(m_process,
            &QProcess::errorOccurred,
            this,
            &InferenceController::handleProcessError);
    connect(m_process, &QProcess::started, this, [this]() {
        emit logReceived(QStringLiteral("RKNN推理程序已启动，等待就绪"));
    });

    connect(m_networkProcess,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &InferenceController::handleNetworkFinished);
    connect(m_networkProcess,
            &QProcess::errorOccurred,
            this,
            &InferenceController::handleNetworkError);

    connect(m_videoReceiver,
            &GstVideoReceiver::frameReady,
            this,
            &InferenceController::handleFrame,
            Qt::AutoConnection);
    connect(m_videoReceiver,
            &GstVideoReceiver::firstFrameReceived,
            this,
            &InferenceController::handleFirstFrame,
            Qt::QueuedConnection);
    connect(m_videoReceiver,
            &GstVideoReceiver::streamError,
            this,
            &InferenceController::handleStreamError,
            Qt::QueuedConnection);

    connect(m_startupTimer,
            &QTimer::timeout,
            this,
            &InferenceController::handleStartupTimeout);
    connect(m_stopTimer,
            &QTimer::timeout,
            this,
            &InferenceController::handleStopTimeout);
    connect(m_videoWatchdog,
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

    QProcess *processes[] = {m_networkProcess, m_process};
    for (QProcess *process : processes) {
        if (process->state() == QProcess::NotRunning) {
            continue;
        }
        process->terminate();
        if (!process->waitForFinished(1000)) {
            process->kill();
            process->waitForFinished(1000);
        }
    }
}

void InferenceController::startDetection()
{
    if (m_state == State::Starting
            || m_state == State::Running
            || m_state == State::Stopping) {
        return;
    }

    if (hasManagedResources()) {
        beginCleanup(
            CleanupOutcome::StartSelected,
            QStringLiteral("正在清理上一次视频来源"));
        return;
    }
    startSelectedMode();
}

void InferenceController::startSelectedMode()
{
    if (m_selectedMode == CameraMode::VisibleLight) {
        startVisibleLight();
    } else if (m_selectedMode == CameraMode::Thermal) {
        startThermal();
    } else {
        enterError(QStringLiteral("尚未选择摄像头模式"));
    }
}

void InferenceController::startVisibleLight()
{
    cleanupReceiver();

    const LaunchSpecResult launch = buildInferenceLaunchSpec(
        m_profile,
        QCoreApplication::applicationDirPath());
    if (!launch.ok) {
        enterError(launch.error);
        return;
    }

    m_activeMode = CameraMode::VisibleLight;
    m_process->setWorkingDirectory(launch.spec.workingDirectory);
    m_process->setProgram(launch.spec.program);
    m_process->setArguments(launch.spec.arguments);
    m_process->setProcessChannelMode(QProcess::SeparateChannels);

    setState(State::Starting, QStringLiteral("正在启动可见光推理程序"));
    m_startupTimer->start();
    m_process->start();
}

void InferenceController::startThermal()
{
    cleanupReceiver();

    QString errorMessage;
    ThermalCameraConfig config;
    if (!loadThermalConfig(config, errorMessage)) {
        enterError(errorMessage);
        return;
    }

    const QString ifconfigProgram = findIfconfig();
    if (ifconfigProgram.isEmpty()) {
        enterError(QStringLiteral(
            "找不到 ifconfig，请安装 net-tools 后重试"));
        return;
    }

    m_thermalConfig = config;
    m_activeMode = CameraMode::Thermal;
    setState(State::Starting, QStringLiteral("正在配置热像仪有线网络"));
    m_startupTimer->start();

    m_networkProcess->setProgram(ifconfigProgram);
    m_networkProcess->setArguments({
        config.interfaceName,
        config.address,
        QStringLiteral("netmask"),
        config.netmask,
        QStringLiteral("up")});
    m_networkProcess->setProcessChannelMode(QProcess::SeparateChannels);
    m_networkProcess->start();
}

bool InferenceController::loadThermalConfig(
    ThermalCameraConfig &config,
    QString &errorMessage) const
{
    const QString configPath = QCoreApplication::applicationDirPath()
        + QStringLiteral("/thermal_camera.ini");
    const QFileInfo fileInfo(configPath);
    if (!fileInfo.exists() || !fileInfo.isReadable()) {
        errorMessage = QStringLiteral(
            "缺少热像仪配置文件：thermal_camera.ini，请参考 thermal_camera.example.ini");
        return false;
    }

    QSettings settings(configPath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("ThermalCamera"));
    config.interfaceName = settings.value(QStringLiteral("interface"))
        .toString().trimmed();
    config.address = settings.value(QStringLiteral("address"))
        .toString().trimmed();
    config.netmask = settings.value(QStringLiteral("netmask"))
        .toString().trimmed();
    config.url = settings.value(QStringLiteral("url"))
        .toString().trimmed();
    bool latencyOk = false;
    config.latency = settings.value(QStringLiteral("latency"), 0)
        .toInt(&latencyOk);
    settings.endGroup();

    if (settings.status() != QSettings::NoError) {
        errorMessage = QStringLiteral("无法读取热像仪配置文件");
        return false;
    }
    if (config.interfaceName.isEmpty()) {
        errorMessage = QStringLiteral("热像仪配置缺少 interface");
        return false;
    }
    if (config.address.isEmpty()) {
        errorMessage = QStringLiteral("热像仪配置缺少 address");
        return false;
    }
    if (config.netmask.isEmpty()) {
        errorMessage = QStringLiteral("热像仪配置缺少 netmask");
        return false;
    }
    if (config.url.isEmpty()) {
        errorMessage = QStringLiteral("热像仪配置缺少 url");
        return false;
    }

    const QUrl url(config.url);
    if (!url.isValid()
            || (url.scheme().compare(QStringLiteral("rtsp"), Qt::CaseInsensitive) != 0
                && url.scheme().compare(QStringLiteral("rtsps"), Qt::CaseInsensitive) != 0)
            || url.host().isEmpty()) {
        errorMessage = QStringLiteral("热像仪 RTSP 地址格式无效");
        return false;
    }
    if (!latencyOk || config.latency < 0) {
        errorMessage = QStringLiteral("热像仪 latency 必须是非负整数");
        return false;
    }
    return true;
}

QString InferenceController::findIfconfig() const
{
    const QString fromPath = QStandardPaths::findExecutable(
        QStringLiteral("ifconfig"));
    if (!fromPath.isEmpty()) {
        return fromPath;
    }

    const QString candidates[] = {
        QStringLiteral("/sbin/ifconfig"),
        QStringLiteral("/usr/sbin/ifconfig")};
    for (const QString &candidate : candidates) {
        const QFileInfo info(candidate);
        if (info.exists() && info.isExecutable()) {
            return candidate;
        }
    }
    return QString();
}

QString InferenceController::networkFailureMessage(
    const QString &standardError) const
{
    const QString safeError = redactRtspUrls(standardError.trimmed());
    const QString lower = safeError.toLower();
    if (lower.contains(QStringLiteral("no such device"))
            || lower.contains(QStringLiteral("device not found"))) {
        return QStringLiteral("不存在有线网卡 %1")
            .arg(m_thermalConfig.interfaceName);
    }
    if (lower.contains(QStringLiteral("operation not permitted"))
            || lower.contains(QStringLiteral("permission denied"))) {
        return QStringLiteral(
            "配置热像仪网卡权限不足，程序需要 root 或 CAP_NET_ADMIN 权限");
    }
    if (safeError.isEmpty()) {
        return QStringLiteral("热像仪网卡 IP 配置失败");
    }
    return QStringLiteral("热像仪网卡 IP 配置失败：%1").arg(safeError);
}

void InferenceController::selectCameraMode(CameraMode mode)
{
    if (mode == CameraMode::None) {
        return;
    }

    const bool changed = mode != m_selectedMode;
    m_selectedMode = mode;
    if (!changed) {
        if (m_state == State::Idle || m_state == State::Error) {
            emit selectedModeChanged(mode, selectedModeMessage(mode));
        }
        return;
    }

    if (m_cleanupInProgress) {
        m_cleanupOutcome = CleanupOutcome::Idle;
        m_cleanupMessage = selectedModeMessage(mode);
        emit selectedModeChanged(
            mode,
            QStringLiteral("正在切换摄像头模式，请稍候"));
        return;
    }

    if (hasManagedResources()
            || m_state == State::Starting
            || m_state == State::Running) {
        emit selectedModeChanged(
            mode,
            QStringLiteral("正在停止当前视频并切换模式"));
        beginCleanup(CleanupOutcome::Idle, selectedModeMessage(mode));
        return;
    }

    setState(State::Idle, selectedModeMessage(mode));
    emit selectedModeChanged(mode, selectedModeMessage(mode));
}

void InferenceController::readProcessOutput()
{
    while (m_process->canReadLine()) {
        const QByteArray line = m_process->readLine().trimmed();
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

void InferenceController::parseProtocolLine(const QByteArray &line)
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
    const QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        emit logReceived(QStringLiteral("无效状态消息：%1")
            .arg(QString::fromUtf8(line)));
        return;
    }

    const QJsonObject object = document.object();
    if (prefix == "@status") {
        const QString state = object.value(QStringLiteral("state")).toString();
        if (state == QStringLiteral("ready")
                && m_state == State::Starting
                && m_activeMode == CameraMode::VisibleLight) {
            if (!m_videoReceiver->start(VideoSource::TcpJpeg)) {
                enterError(QStringLiteral("TCP 视频接收启动失败"));
            }
        }
        return;
    }

    if (prefix == "@metrics") {
        if (m_activeMode != CameraMode::VisibleLight
                || m_state != State::Running) {
            return;
        }
        emit metricsUpdated(
            object.value(QStringLiteral("pipeline_fps")).toDouble(),
            object.value(QStringLiteral("latency_ms")).toDouble());
        return;
    }

    if (prefix == "@error") {
        enterError(object.value(QStringLiteral("message"))
            .toString(QStringLiteral("推理程序发生错误")));
    }
}

void InferenceController::handleNetworkFinished(
    int exitCode,
    QProcess::ExitStatus exitStatus)
{
    const QString standardError = QString::fromUtf8(
        m_networkProcess->readAllStandardError());

    if (m_cleanupInProgress) {
        maybeCompleteCleanup();
        return;
    }
    if (m_activeMode != CameraMode::Thermal || m_state != State::Starting) {
        return;
    }
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        enterError(networkFailureMessage(standardError));
        return;
    }

    emit logReceived(QStringLiteral("热像仪网卡配置完成，正在连接视频流"));
    setState(State::Starting, QStringLiteral("正在连接热像仪视频流"));
    if (!m_videoReceiver->start(
            VideoSource::ThermalRtsp,
            m_thermalConfig.url,
            m_thermalConfig.latency)) {
        enterError(QStringLiteral("热像仪 RTSP 接收启动失败"));
    }
}

void InferenceController::handleNetworkError(QProcess::ProcessError error)
{
    if (m_cleanupInProgress) {
        maybeCompleteCleanup();
        return;
    }
    if (error == QProcess::FailedToStart) {
        enterError(QStringLiteral("无法启动 ifconfig：%1")
            .arg(m_networkProcess->errorString()));
    } else if (error == QProcess::Crashed) {
        enterError(QStringLiteral("ifconfig 进程异常退出"));
    }
}

void InferenceController::handleFrame(const QImage &frame)
{
    if (m_activeMode == CameraMode::None
            || (m_state != State::Starting && m_state != State::Running)) {
        return;
    }
    m_lastFrameTimer.restart();
    emit frameReady(frame);
}

void InferenceController::handleFirstFrame()
{
    if (m_state != State::Starting || m_activeMode == CameraMode::None) {
        return;
    }

    m_startupTimer->stop();
    m_lastFrameTimer.start();
    m_videoWatchdog->start();
    setState(
        State::Running,
        m_activeMode == CameraMode::Thermal
            ? QStringLiteral("热像仪画面运行中")
            : QStringLiteral("正在检测"));
}

void InferenceController::stopDetection()
{
    if (m_state == State::Idle && !hasManagedResources()) {
        return;
    }
    if (m_cleanupInProgress) {
        m_cleanupOutcome = CleanupOutcome::Idle;
        m_cleanupMessage = QStringLiteral("等待开始");
        return;
    }
    beginCleanup(CleanupOutcome::Idle, QStringLiteral("等待开始"));
}

void InferenceController::beginCleanup(
    CleanupOutcome outcome,
    const QString &message)
{
    m_cleanupOutcome = outcome;
    m_cleanupMessage = message;

    if (m_cleanupInProgress) {
        if (outcome == CleanupOutcome::Error) {
            setState(State::Error, message);
        }
        return;
    }

    m_cleanupInProgress = true;
    m_startupTimer->stop();
    m_videoWatchdog->stop();
    m_lastFrameTimer.invalidate();
    cleanupReceiver();

    if (outcome == CleanupOutcome::Error) {
        setState(State::Error, message);
    } else {
        setState(State::Stopping, QStringLiteral("正在停止当前视频来源"));
    }

    if (m_networkProcess->state() != QProcess::NotRunning) {
        m_networkProcess->terminate();
    }
    if (m_process->state() != QProcess::NotRunning) {
        m_process->terminate();
    }

    if (m_networkProcess->state() != QProcess::NotRunning
            || m_process->state() != QProcess::NotRunning) {
        m_stopTimer->start();
    } else {
        completeCleanup();
    }
}

void InferenceController::handleStopTimeout()
{
    if (m_networkProcess->state() != QProcess::NotRunning) {
        m_networkProcess->kill();
    }
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
    maybeCompleteCleanup();
}

void InferenceController::handleProcessFinished(
    int exitCode,
    QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)

    if (m_cleanupInProgress) {
        maybeCompleteCleanup();
        return;
    }

    m_startupTimer->stop();
    m_videoWatchdog->stop();
    cleanupReceiver();

    if (m_activeMode != CameraMode::VisibleLight) {
        return;
    }

    const QString message = exitStatus == QProcess::CrashExit
        ? QStringLiteral("推理程序异常退出")
        : QStringLiteral("推理程序已退出");
    enterError(message);
}

void InferenceController::handleProcessError(QProcess::ProcessError error)
{
    if (m_cleanupInProgress) {
        maybeCompleteCleanup();
        return;
    }

    switch (error) {
    case QProcess::FailedToStart:
        enterError(QStringLiteral("无法启动推理程序"));
        break;
    case QProcess::Crashed:
        enterError(QStringLiteral("推理程序崩溃"));
        break;
    default:
        enterError(m_process->errorString());
        break;
    }
}

void InferenceController::handleStreamError(const QString &message)
{
    if (m_cleanupInProgress
            || m_state == State::Stopping
            || m_state == State::Idle
            || m_state == State::Error) {
        return;
    }
    enterError(QStringLiteral("视频连接异常：%1").arg(message));
}

void InferenceController::checkVideoTimeout()
{
    if (m_state != State::Running || !m_lastFrameTimer.isValid()) {
        return;
    }
    if (m_lastFrameTimer.elapsed() > 3000) {
        enterError(QStringLiteral("超过 3 秒未收到视频帧"));
    }
}

void InferenceController::handleStartupTimeout()
{
    if (m_state != State::Starting) {
        return;
    }
    enterError(
        m_activeMode == CameraMode::Thermal
            ? QStringLiteral("热像仪网络配置或视频连接超时")
            : QStringLiteral("推理程序启动超时"));
}

void InferenceController::enterError(const QString &message)
{
    beginCleanup(CleanupOutcome::Error, redactRtspUrls(message));
}

void InferenceController::maybeCompleteCleanup()
{
    if (!m_cleanupInProgress) {
        return;
    }
    if (m_networkProcess->state() == QProcess::NotRunning
            && m_process->state() == QProcess::NotRunning) {
        completeCleanup();
    }
}

void InferenceController::completeCleanup()
{
    if (!m_cleanupInProgress) {
        return;
    }

    m_stopTimer->stop();
    cleanupReceiver();
    m_activeMode = CameraMode::None;

    const CleanupOutcome outcome = m_cleanupOutcome;
    const QString message = m_cleanupMessage;
    m_cleanupInProgress = false;
    m_cleanupOutcome = CleanupOutcome::Idle;
    m_cleanupMessage.clear();

    if (outcome == CleanupOutcome::Error) {
        setState(State::Error, message);
        return;
    }

    if (outcome == CleanupOutcome::StartSelected) {
        setState(State::Idle, QStringLiteral("清理完成，正在重新启动"));
        QTimer::singleShot(0, this, &InferenceController::startDetection);
        return;
    }

    const QString idleMessage = message.isEmpty()
        ? QStringLiteral("等待开始")
        : message;
    setState(State::Idle, idleMessage);
    emit selectedModeChanged(m_selectedMode, idleMessage);
}

bool InferenceController::hasManagedResources() const
{
    return m_activeMode != CameraMode::None
        || m_videoReceiver->isRunning()
        || m_process->state() != QProcess::NotRunning
        || m_networkProcess->state() != QProcess::NotRunning;
}

QString InferenceController::selectedModeMessage(CameraMode mode) const
{
    return mode == CameraMode::Thermal
        ? QStringLiteral("已选择热像仪模式，点击开始检测")
        : QStringLiteral("已选择可见光摄像头，点击开始检测");
}

void InferenceController::setState(State state, const QString &message)
{
    m_state = state;
    emit stateChanged(state, message);
}

InferenceController::State InferenceController::state() const
{
    return m_state;
}

CameraMode InferenceController::selectedMode() const
{
    return m_selectedMode;
}

CameraMode InferenceController::activeMode() const
{
    return m_activeMode;
}

void InferenceController::cleanupReceiver()
{
    if (m_videoReceiver != nullptr) {
        m_videoReceiver->stop();
    }
}
