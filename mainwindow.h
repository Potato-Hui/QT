#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "cameramode.h"
#include "gpiolightcontroller.h"
#include "photoarchive.h"
#include "snapshotprotocol.h"

#include <QImage>
#include <QMainWindow>
#include <QPixmap>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QTimer;
class QResizeEvent;
class QThread;
class QuantificationService;
class QJsonObject;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    void setPreviewFrame(const QImage &frame);
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
    void setSelectedCameraMode(
        CameraMode mode,
        const QString &message = QString());
    void processSnapshotPackage(const SnapshotPackage& package);
    void handleSnapshotFailure(const QString& requestId, const QString& message);

signals:
    void detectionStartRequested();
    void detectionStopRequested();
    void snapshotRequested(const SnapshotRequest& request);
    void quantificationRequested(const SnapshotPackage& package);
    void settingsRequested();
    void cameraModeSelectionRequested(CameraMode mode);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateClock();
    void toggleDetection();
    void openRecordsPage();
    void clearAllPhotos();
    void openMonitorPage();
    void requestSnapshot();
    void updateStorageSpace();
    void openPhotoDetail(int row, int column);
    void backToRecordsPage();
    void zoomDetailIn();
    void zoomDetailOut();
    void resetDetailZoom();
    void fitDetailImage();
    void viewCurrentJson();
    void quantifyCurrentPhoto();
    void exportCurrentPhoto();
    void deleteCurrentPhoto();
    void openSettingsPage();
    void toggleLight();
    void handleQuantificationCompleted(const SnapshotPackage& package,
                                       const QJsonObject& result);
    void handleQuantificationFailed(const SnapshotPackage& package,
                                    const QString& message);
private:
    void updateDetectionButton();
    void updatePreviewPixmap();
    void refreshHistoryPhotos();
    void updateDetailPixmap();
    void clearDetailState();
    void updateSnapshotButton();
    void updateInstitutionLogo();
    void updateLightUi(const QString& statusMessage = QString());
    void moveFailedRecord(const SnapshotPackage& package);
    double detailFitScale() const;

    Ui::MainWindow *ui;
    QTimer *m_clockTimer;
    QTimer *m_storageTimer;
    QString m_storagePath;
    PhotoArchive m_photoArchive;
    QThread* m_quantificationThread;
    QuantificationService* m_quantificationService;
    GpioLightController m_lightController;
    bool m_detecting;
    bool m_snapshotPending;
    QString m_pendingSnapshotId;
    QImage m_lastFrame;
    QString m_currentPhotoPath;
    QPixmap m_detailPixmap;
    double m_detailScale;
    bool m_detailFitMode;
};

#endif // MAINWINDOW_H
