#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "photoarchive.h"

#include <QImage>
#include <QMainWindow>
#include <QPixmap>
#include <QString>

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
    void updateStorageSpace();
    void openPhotoDetail(int row, int column);
    void backToRecordsPage();
    void zoomDetailIn();
    void zoomDetailOut();
    void resetDetailZoom();
    void fitDetailImage();
    void exportCurrentPhoto();
    void deleteCurrentPhoto();
    void openSettingsPage();
private:
    void updateDetectionButton();
    void updatePreviewPixmap();
    void refreshHistoryPhotos();
    void updateDetailPixmap();
    void clearDetailState();
    double detailFitScale() const;

    Ui::MainWindow *ui;
    QTimer *m_clockTimer;
    QTimer *m_storageTimer;
    QString m_storagePath;
    PhotoArchive m_photoArchive;
    bool m_detecting;
    QImage m_lastFrame;
    QString m_currentPhotoPath;
    QPixmap m_detailPixmap;
    double m_detailScale;
    bool m_detailFitMode;
};

#endif // MAINWINDOW_H
