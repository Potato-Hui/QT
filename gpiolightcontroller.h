#ifndef GPIOLIGHTCONTROLLER_H
#define GPIOLIGHTCONTROLLER_H

#include <QString>

#ifndef LIGHT_GPIO_CHIP_PATH
#define LIGHT_GPIO_CHIP_PATH "/dev/gpiochip4"
#endif

#ifndef LIGHT_GPIO_LINE_OFFSET
#define LIGHT_GPIO_LINE_OFFSET 4U
#endif

#ifndef LIGHT_GPIO_ACTIVE_LEVEL
#define LIGHT_GPIO_ACTIVE_LEVEL 1
#endif

class GpioLightController
{
public:
    GpioLightController();
    ~GpioLightController();

    GpioLightController(const GpioLightController&) = delete;
    GpioLightController& operator=(const GpioLightController&) = delete;

    bool initialize(QString* errorMessage = nullptr);
    bool setLightEnabled(bool enabled, QString* errorMessage = nullptr);
    bool isLightEnabled() const;
    void shutdown();

private:
    static int physicalValue(bool enabled);
    static QString gpioError(const QString& action, int errorNumber);
    static void setError(QString* errorMessage, const QString& message);

    int m_lineFd;
    bool m_initialized;
    bool m_enabled;
};

#endif // GPIOLIGHTCONTROLLER_H
