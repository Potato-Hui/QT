#include "mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>

#include <cassert>

template <typename Widget>
Widget* required(MainWindow& window, const char* objectName)
{
    Widget* widget = window.findChild<Widget*>(QString::fromLatin1(objectName));
    assert(widget != nullptr);
    return widget;
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QFile styleFile(QStringLiteral(":/styles/white_theme.qss"));
    assert(styleFile.open(QIODevice::ReadOnly | QIODevice::Text));
    const QByteArray style = styleFile.readAll();
    app.setStyleSheet(QString::fromUtf8(style));

    MainWindow window;

    QStackedWidget* pages = required<QStackedWidget>(window, "pageStack");
    assert(pages->count() == 3);
    assert(required<QWidget>(window, "monitorPage") != nullptr);
    assert(required<QWidget>(window, "recordsPage") != nullptr);
    assert(required<QWidget>(window, "photoDetailPage") != nullptr);

    required<QLabel>(window, "detailImageLabel");
    required<QLabel>(window, "detailFileNameLabel");
    required<QLabel>(window, "detailFileSizeValueLabel");
    required<QLabel>(window, "detailImageSizeValueLabel");
    required<QLabel>(window, "detailPathValueLabel");
    required<QPushButton>(window, "backToRecordsButton");
    required<QPushButton>(window, "zoomOutButton");
    required<QPushButton>(window, "zoomResetButton");
    required<QPushButton>(window, "zoomInButton");
    required<QPushButton>(window, "fitImageButton");
    required<QPushButton>(window, "exportPhotoButton");
    required<QPushButton>(window, "deletePhotoButton");
    required<QPushButton>(window, "clearRecordsButton");
    required<QLabel>(window, "recordsCountLabel");

    assert(window.findChild<QWidget*>("batteryProgress") == nullptr);
    assert(window.findChild<QWidget*>("batteryValueLabel") == nullptr);
    assert(window.findChild<QWidget*>("deviceStatusDot") == nullptr);
    assert(window.findChild<QWidget*>("deviceStatusLabel") == nullptr);
    assert(window.findChild<QWidget*>("recordsNavButton") == nullptr);

    assert(style.contains("QLabel[role=\"pageTitle\"]"));
    assert(style.contains("QPushButton[dangerOutline=\"true\"]"));
    assert(style.contains("QPushButton[primaryBlue=\"true\"]"));
    assert(style.contains("QWidget#photoDetailPage"));

    const QString screenshotPath = qEnvironmentVariable("UI_SCREENSHOT_PATH");
    if (!screenshotPath.isEmpty()) {
        window.resize(1280, 720);
        window.show();
        app.processEvents();
        assert(window.grab().save(screenshotPath));
    }

    return 0;
}
