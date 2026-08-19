#include "monitorapplication.h"
#include "inferencecontroller.h"
#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <gst/gst.h>

int runMonitorApplication(int argc, char *argv[], InferenceProfile profile)
{
    gst_init(&argc, &argv);
    QApplication app(argc, argv);
    app.setApplicationName(profile == InferenceProfile::SingleModel
        ? QStringLiteral("绝缘子单模型检测系统")
        : QStringLiteral("绝缘子智能检测系统"));
    app.setOrganizationName(QStringLiteral("Insulator Monitor"));

    QFile styleFile(QStringLiteral(":/styles/white_theme.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&styleFile);
        app.setStyleSheet(stream.readAll());
    }

    MainWindow window;
    InferenceController controller(profile);
    QObject::connect(&controller, &InferenceController::frameReady,
                     &window, &MainWindow::setPreviewFrame, Qt::AutoConnection);
    QObject::connect(&window, &MainWindow::detectionStartRequested,
                     &controller, &InferenceController::startDetection);
    QObject::connect(&window, &MainWindow::detectionStopRequested,
                     &controller, &InferenceController::stopDetection);
    qRegisterMetaType<SnapshotRequest>("SnapshotRequest");
    qRegisterMetaType<SnapshotPackage>("SnapshotPackage");
    QObject::connect(&window, &MainWindow::snapshotRequested,
                     &controller, &InferenceController::requestSnapshot);
    QObject::connect(&controller, &InferenceController::snapshotReady,
                     &window, &MainWindow::processSnapshotPackage);
    QObject::connect(&controller, &InferenceController::snapshotFailed,
                     &window, &MainWindow::handleSnapshotFailure);
    QObject::connect(&controller, &InferenceController::stateChanged,
        &window, [&window](InferenceController::State state, const QString& message) {
            switch (state) {
            case InferenceController::State::Starting: window.setDetectionUiState(false, true, false, message); break;
            case InferenceController::State::Running: window.setDetectionUiState(true, false, false, message); break;
            case InferenceController::State::Stopping: window.setDetectionUiState(true, true, false, message); break;
            case InferenceController::State::Error: window.setDetectionUiState(false, false, true, message); break;
            case InferenceController::State::Idle: window.setDetectionUiState(false, false, false, message); break;
            }
        });
    QObject::connect(&controller, &InferenceController::logReceived,
                     [](const QString& message) { qInfo().noquote() << message; });
    QObject::connect(&controller, &InferenceController::metricsUpdated,
                     &window, &MainWindow::setPerformanceMetrics);
    window.showFullScreen();
    return app.exec();
}
