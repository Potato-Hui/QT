#include "mainwindow.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLinearGradient>
#include <QPainter>
#include <QTextStream>
#include <QTimer>

namespace {

QImage createDemoFrame()
{
    QImage frame(1280, 720, QImage::Format_RGB32);
    QPainter painter(&frame);

    QLinearGradient background(0, 0, frame.width(), frame.height());
    background.setColorAt(0.0, QColor("#203A5E"));
    background.setColorAt(1.0, QColor("#0D1828"));
    painter.fillRect(frame.rect(), background);

    painter.setPen(QPen(QColor("#4FC3F7"), 4));
    painter.drawRect(340, 145, 550, 420);
    painter.setPen(QColor("#FFFFFF"));
    painter.setFont(QFont("Arial", 28, QFont::Bold));
    painter.drawText(frame.rect(), Qt::AlignCenter,
        QStringLiteral("Desktop UI Preview"));
    painter.setFont(QFont("Arial", 16));
    painter.drawText(360, 610,
        QStringLiteral("No camera or RK3588 inference is running"));

    return frame;
}

void showRunningState(MainWindow *window)
{
    window->setPreviewFrame(createDemoFrame());
    window->setDetectionUiState(true, false, false,
        QStringLiteral("正在检测"));
    window->setDetectionResult(QStringLiteral("正常"), 0.987, 1280 * 720, 0);
    window->setPerformanceMetrics(29.8, 34.0);
}

void createDemoRecord(const SnapshotRequest& request, MainWindow* window)
{
    const QDir recordDir(request.outputDir);
    const SnapshotPackage package{request.requestId, request.outputDir};
    QDir().mkpath(recordDir.filePath(QStringLiteral("masks")));

    const QImage image = createDemoFrame();
    if (!image.save(recordDir.filePath(QStringLiteral("image.jpg")), "JPG", 90)) {
        window->handleSnapshotFailure(request.requestId, QStringLiteral("无法保存预览原图"));
        return;
    }

    const QRect bbox(340, 145, 550, 420);
    QImage mask(bbox.size(), QImage::Format_Grayscale8);
    mask.fill(255);
    if (!mask.save(recordDir.filePath(QStringLiteral("masks/insulator_01.png")), "PNG")) {
        window->handleSnapshotFailure(request.requestId, QStringLiteral("无法保存预览 mask"));
        return;
    }

    QJsonObject instance;
    instance.insert(QStringLiteral("instance_id"), 1);
    instance.insert(QStringLiteral("class_id"), 0);
    instance.insert(QStringLiteral("class_name"), QStringLiteral("insulator"));
    instance.insert(QStringLiteral("confidence"), 0.987);
    instance.insert(QStringLiteral("bbox_xyxy"), QJsonArray{bbox.left(), bbox.top(), bbox.right() + 1, bbox.bottom() + 1});
    instance.insert(QStringLiteral("mask_file"), QStringLiteral("masks/insulator_01.png"));

    QJsonObject metadata;
    metadata.insert(QStringLiteral("image_width"), image.width());
    metadata.insert(QStringLiteral("image_height"), image.height());
    metadata.insert(QStringLiteral("instances"), QJsonArray{instance});
    QFile metadataFile(recordDir.filePath(QStringLiteral("metadata.json")));
    if (!metadataFile.open(QIODevice::WriteOnly)) {
        window->handleSnapshotFailure(request.requestId, QStringLiteral("无法保存预览 metadata"));
        return;
    }
    metadataFile.write(QJsonDocument(metadata).toJson(QJsonDocument::Indented));

    window->processSnapshotPackage(package);
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("绝缘子智能检测系统"));

    QFile styleFile(QStringLiteral(":/styles/white_theme.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
    }

    MainWindow window;
    qRegisterMetaType<SnapshotPackage>("SnapshotPackage");
    window.setDetectionUiState(false, false, false,
        QStringLiteral("等待开始"));

    QObject::connect(&window, &MainWindow::detectionStartRequested,
        &window, [&window] { showRunningState(&window); });
    QObject::connect(&window, &MainWindow::snapshotRequested,
        &window, [&window](const SnapshotRequest& request) {
            createDemoRecord(request, &window);
        });
    QObject::connect(&window, &MainWindow::detectionStopRequested,
        &window, [&window] {
            window.setDetectionUiState(false, false, false,
                QStringLiteral("等待开始"));
        });

    QTimer metricsTimer;
    QObject::connect(&metricsTimer, &QTimer::timeout, &window, [&window] {
        const qint64 milliseconds = QDateTime::currentMSecsSinceEpoch();
        const double fps = 29.5 + (milliseconds % 10) / 10.0;
        const double latency = 30.0 + (milliseconds % 7);
        window.setPerformanceMetrics(fps, latency);
    });
    metricsTimer.start(1000);

    window.showFullScreen();
    return app.exec();
}
