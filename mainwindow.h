#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QImage>
#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QTimer;
class QResizeEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void setPreviewFrame(const QImage &frame);
    void setBatteryLevel(int percent, bool charging = false);
    void setDeviceStatus(bool cameraReady, bool modelReady);
    void setDetectionResult(const QString &result,
                            double confidence,
                            qint64 totalPixels,
                            qint64 defectPixels);
    void setDetectionUiState(bool detecting,
                             bool busy,
                             bool error,
                             const QString &message);
    void setPerformanceMetrics(double pipelineFps,
                               double latencyMs);

signals:
    void detectionStartRequested();
    void detectionStopRequested();
    void snapshotRequested();
    void settingsRequested();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateClock();
    void toggleDetection();
    void openRecordsPage();
    void openMonitorPage();
    void requestSnapshot();

private:
    void updateDetectionButton();
    void updatePreviewPixmap();

    Ui::MainWindow *ui;
    QTimer *m_clockTimer;
    bool m_detecting;
    QImage m_lastFrame;
};

#endif // MAINWINDOW_H
