#include "mainwindow.h"

#include <QApplication>
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

    return 0;
}
