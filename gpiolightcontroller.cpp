#include "gpiolightcontroller.h"

#include <cerrno>
#include <QtGlobal>

#if defined(Q_OS_LINUX) && !defined(INSULATOR_DESKTOP_PREVIEW)
#include <cstring>
#include <fcntl.h>
#include <linux/gpio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

static_assert(LIGHT_GPIO_ACTIVE_LEVEL == 0 || LIGHT_GPIO_ACTIVE_LEVEL == 1,
              "LIGHT_GPIO_ACTIVE_LEVEL must be 0 or 1");

GpioLightController::GpioLightController()
    : m_lineFd(-1)
    , m_initialized(false)
    , m_enabled(false)
{
}

GpioLightController::~GpioLightController()
{
    shutdown();
}

bool GpioLightController::initialize(QString* errorMessage)
{
    if (m_initialized) {
        return true;
    }

#ifdef INSULATOR_DESKTOP_PREVIEW
    Q_UNUSED(errorMessage);
    m_initialized = true;
    m_enabled = false;
    return true;
#elif defined(Q_OS_LINUX)
    const int chipFd = ::open(LIGHT_GPIO_CHIP_PATH, O_RDONLY | O_CLOEXEC);
    if (chipFd < 0) {
        setError(errorMessage,
                 gpioError(QStringLiteral("无法打开GPIO设备 %1")
                               .arg(QStringLiteral(LIGHT_GPIO_CHIP_PATH)),
                           errno));
        return false;
    }

    struct gpiohandle_request request;
    std::memset(&request, 0, sizeof(request));
    request.lineoffsets[0] = LIGHT_GPIO_LINE_OFFSET;
    request.flags = GPIOHANDLE_REQUEST_OUTPUT;
    request.default_values[0] = physicalValue(false);
    request.lines = 1;
    std::strncpy(request.consumer_label, "insulator-light",
                 sizeof(request.consumer_label) - 1);

    if (::ioctl(chipFd, GPIO_GET_LINEHANDLE_IOCTL, &request) < 0) {
        const int requestError = errno;
        ::close(chipFd);
        setError(errorMessage,
                 gpioError(QStringLiteral("无法申请GPIO4_A4（%1，line %2）")
                               .arg(QStringLiteral(LIGHT_GPIO_CHIP_PATH))
                               .arg(LIGHT_GPIO_LINE_OFFSET),
                           requestError));
        return false;
    }

    ::close(chipFd);
    m_lineFd = request.fd;
    m_initialized = true;
    m_enabled = false;
    return true;
#else
    setError(errorMessage, QStringLiteral("当前平台不支持RK3588 GPIO控制"));
    return false;
#endif
}

bool GpioLightController::setLightEnabled(bool enabled, QString* errorMessage)
{
    if (!m_initialized && !initialize(errorMessage)) {
        return false;
    }

#ifdef INSULATOR_DESKTOP_PREVIEW
    m_enabled = enabled;
    return true;
#elif defined(Q_OS_LINUX)
    struct gpiohandle_data data;
    std::memset(&data, 0, sizeof(data));
    data.values[0] = physicalValue(enabled);
    if (::ioctl(m_lineFd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data) < 0) {
        setError(errorMessage,
                 gpioError(enabled ? QStringLiteral("GPIO输出高电平失败")
                                   : QStringLiteral("GPIO输出低电平失败"),
                           errno));
        return false;
    }

    m_enabled = enabled;
    return true;
#else
    Q_UNUSED(enabled);
    setError(errorMessage, QStringLiteral("当前平台不支持RK3588 GPIO控制"));
    return false;
#endif
}

bool GpioLightController::isLightEnabled() const
{
    return m_enabled;
}

void GpioLightController::shutdown()
{
    if (!m_initialized) {
        return;
    }

#ifdef INSULATOR_DESKTOP_PREVIEW
    m_enabled = false;
#elif defined(Q_OS_LINUX)
    struct gpiohandle_data data;
    std::memset(&data, 0, sizeof(data));
    data.values[0] = physicalValue(false);
    if (m_lineFd >= 0) {
        ::ioctl(m_lineFd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
        ::close(m_lineFd);
    }
    m_lineFd = -1;
    m_enabled = false;
#endif

    m_initialized = false;
}

int GpioLightController::physicalValue(bool enabled)
{
    return enabled ? LIGHT_GPIO_ACTIVE_LEVEL : 1 - LIGHT_GPIO_ACTIVE_LEVEL;
}

QString GpioLightController::gpioError(const QString& action, int errorNumber)
{
    QString reason;
    switch (errorNumber) {
    case ENOENT:
        reason = QStringLiteral("GPIO设备不存在，请确认gpiochip路径和设备树");
        break;
    case EACCES:
    case EPERM:
        reason = QStringLiteral("权限不足，请使用root、GPIO udev规则或授予设备访问权限");
        break;
    case EBUSY:
        reason = QStringLiteral("GPIO线路已被其他驱动或进程占用");
        break;
    case EINVAL:
        reason = QStringLiteral("GPIO line offset或输出参数无效");
        break;
    default:
#if defined(Q_OS_LINUX) && !defined(INSULATOR_DESKTOP_PREVIEW)
        reason = QString::fromLocal8Bit(std::strerror(errorNumber));
#else
        reason = QStringLiteral("未知错误");
#endif
        break;
    }
    return QStringLiteral("%1：%2").arg(action, reason);
}

void GpioLightController::setError(QString* errorMessage,
                                   const QString& message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}
