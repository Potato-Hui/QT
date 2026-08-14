#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDateTime>
#include <QDir>
#include <QDialog>
#include <QFileInfo>
#include <QFileInfoList>
#include <QHeaderView>
#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <QResizeEvent>
#include <QStorageInfo>
#include <QStyle>
#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_clockTimer(new QTimer(this))
    , m_storageTimer(new QTimer(this))
    , m_storagePath(QDir::homePath() + QStringLiteral("/InsulatorMonitor/data"))
    , m_detecting(false)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("绝缘子智能检测系统"));

    connect(m_clockTimer, &QTimer::timeout, this, &MainWindow::updateClock);
    connect(ui->detectButton, &QPushButton::clicked,
            this, &MainWindow::toggleDetection);
    connect(ui->snapshotButton, &QPushButton::clicked,
            this, &MainWindow::requestSnapshot);
    connect(ui->recordsButton, &QPushButton::clicked,
            this, &MainWindow::openRecordsPage);
    connect(ui->backButton, &QPushButton::clicked,
            this, &MainWindow::openMonitorPage);
    connect(ui->settingsButton, &QPushButton::clicked,
            this, &MainWindow::settingsRequested);
    connect(ui->recordsTable, &QTableWidget::cellClicked,
            this, &MainWindow::openHistoryPhoto);
    //connect(  发送者,        &发送者类名::信号,      接收者,       &接收者类名::槽函数 );
    m_clockTimer->start(1000);
    updateClock();
    QDir().mkpath(m_storagePath);
    connect(m_storageTimer, &QTimer::timeout,
            this, &MainWindow::updateStorageSpace);
    m_storageTimer->start(5000);
    updateStorageSpace();
    updateDetectionButton();
    ui->pageStack->setCurrentWidget(ui->monitorPage);
    auto *recordsHeader = ui->recordsTable->horizontalHeader();
    // Keep the history table columns large enough for an actual thumbnail and
    // readable metadata.  This is scoped to the history page and does not
    // change the realtime preview layout.
    recordsHeader->setSectionResizeMode(0, QHeaderView::Fixed);
    recordsHeader->setSectionResizeMode(1, QHeaderView::Fixed);
    recordsHeader->setSectionResizeMode(2, QHeaderView::Fixed);
    recordsHeader->setSectionResizeMode(3, QHeaderView::Fixed);
    recordsHeader->setStretchLastSection(true);
    //设置0-3列为固定宽度，最后一列自动拉伸
    ui->recordsTable->setIconSize(QSize(180, 112));
    // 设置表格中图标的显示大小为 180×112 像素
    ui->recordsTable->verticalHeader()->setVisible(false);
    ui->recordsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->recordsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    refreshHistoryPhotos();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setPreviewFrame(const QImage &frame)//接受图像，保存并显示
{
    if (frame.isNull()) {
        return;
    }
    m_lastFrame = frame;
    updatePreviewPixmap();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updatePreviewPixmap();
}

void MainWindow::setDetectionResult(const QString &result,
                                    double confidence,
                                    qint64 totalPixels,
                                    qint64 defectPixels)
{
    ui->resultValueLabel->setText(result.isEmpty() ? QStringLiteral("等待检测") : result);
    ui->confidenceValueLabel->setText(QStringLiteral("%1%")
                                         .arg(confidence * 100.0, 0, 'f', 1));

    const double ratio = totalPixels > 0
        ? (100.0 * static_cast<double>(defectPixels) / static_cast<double>(totalPixels))
        : 0.0;

    const bool alarm = result.contains(QStringLiteral("破损"))
                    || result.contains(QStringLiteral("闪络"))
                    || ratio > 0.0;
    ui->resultValueLabel->setProperty("resultState", alarm ? "alarm" : "normal");
    ui->resultValueLabel->style()->unpolish(ui->resultValueLabel);//更新样式
    ui->resultValueLabel->style()->polish(ui->resultValueLabel);
}

void MainWindow::updateClock()
{
    const QDateTime now = QDateTime::currentDateTime();
    ui->timeLabel->setText(now.toString(QStringLiteral("HH:mm:ss")));
    ui->dateLabel->setText(now.toString(QStringLiteral("yyyy年MM月dd日  dddd")));
}

void MainWindow::toggleDetection()//点击开始检测按钮
{
    if (m_detecting) {
        emit detectionStopRequested();//点击关闭界面后，画面应该消失，后面需要修改
    } else {
        emit detectionStartRequested();
    }
}

void MainWindow::setDetectionUiState(bool detecting,
                                     bool busy,
                                     bool error,
                                     const QString &message)
{
    m_detecting = detecting;
    updateDetectionButton();
    ui->detectButton->setEnabled(!busy);

    if (busy) {
        ui->detectButton->setText(detecting
            ? QStringLiteral("正在停止…")
            : QStringLiteral("正在启动…"));
    } else if (error) {
        ui->detectButton->setText(QStringLiteral("重新开始"));
    }

    if (!message.isEmpty()) {
        ui->runStatusLabel->setText(message);
    }
    ui->runStatusDot->setProperty(
        "state", error ? "error" : (detecting ? "running" : "idle"));
    ui->runStatusDot->style()->unpolish(ui->runStatusDot);
    ui->runStatusDot->style()->polish(ui->runStatusDot);
}

void MainWindow::setPerformanceMetrics(double pipelineFps,
                                       double latencyMs)
{
    if (!m_detecting) {
        return;
    }

    ui->fpsValueLabel->setText(QString::number(pipelineFps, 'f', 1));
    ui->latencyValueLabel->setText(
        QStringLiteral("%1 ms").arg(latencyMs, 0, 'f', 0));
    ui->runStatusLabel->setText(
        QStringLiteral("正在检测 · FPS %1 · 延迟 %2 ms")
            .arg(pipelineFps, 0, 'f', 1)
            .arg(latencyMs, 0, 'f', 0));
}

void MainWindow::openRecordsPage()//照片界面
{
    refreshHistoryPhotos();
    ui->pageStack->setCurrentWidget(ui->recordsPage);
}

void MainWindow::openMonitorPage()
{
    ui->pageStack->setCurrentWidget(ui->monitorPage);
}

void MainWindow::requestSnapshot()
{
    if (m_lastFrame.isNull()) {
        QMessageBox::warning(
            this,
            QStringLiteral("拍照失败"),
            QStringLiteral("当前没有可保存的摄像头画面"));
        return;
    }

    QDir().mkpath(m_storagePath);

    const QString fileName =
        QStringLiteral("snapshot_%1.jpg")
        .arg(QDateTime::currentDateTime()
             .toString(QStringLiteral("yyyyMMdd_HHmmss")));
    const QString filePath = QDir(m_storagePath).filePath(fileName);

    if (!m_lastFrame.save(filePath, "JPG", 90)) {
        QMessageBox::critical(
            this,
            QStringLiteral("拍照失败"),
            QStringLiteral("无法保存图片：%1").arg(filePath));
        return;
    }

    emit snapshotRequested();
    const int count = ui->snapshotCountValueLabel->text().toInt() + 1;
    ui->snapshotCountValueLabel->setText(QString::number(count));
    updateStorageSpace();
}

void MainWindow::updateStorageSpace()
{
    const QStorageInfo storage(m_storagePath);
    if (!storage.isValid() || !storage.isReady()) {
        ui->storageValueLabel->setText(QStringLiteral("--"));
        return;
    }

    const double gigabytes = static_cast<double>(storage.bytesAvailable())
        / (1024.0 * 1024.0 * 1024.0);
    ui->storageValueLabel->setText(
        QStringLiteral("%1 GB").arg(gigabytes, 0, 'f', 1));
}

void MainWindow::refreshHistoryPhotos()
{
    QDir directory(m_storagePath);
    const QStringList filters{
        QStringLiteral("*.jpg"),
        QStringLiteral("*.jpeg"),
        QStringLiteral("*.png")};
    const QFileInfoList photos = directory.entryInfoList(
        filters, QDir::Files | QDir::Readable, QDir::Time);

    ui->recordsTable->clearContents();
    ui->recordsTable->setColumnCount(4);
    ui->recordsTable->setHorizontalHeaderLabels({
        QStringLiteral("缩略图"),
        QStringLiteral("文件名"),
        QStringLiteral("拍摄时间"),
        QStringLiteral("文件大小")});
    ui->recordsTable->setRowCount(photos.size());
    ui->recordsTable->setColumnWidth(0, 210);
    ui->recordsTable->setColumnWidth(1, 390);
    ui->recordsTable->setColumnWidth(2, 260);
    ui->recordsTable->setColumnWidth(3, 160);

    for (int row = 0; row < photos.size(); ++row) {
        const QFileInfo &photo = photos.at(row);
        auto *thumbnail = new QTableWidgetItem;
        const QPixmap pixmap(photo.absoluteFilePath());
        if (!pixmap.isNull()) {
            thumbnail->setIcon(QIcon(pixmap.scaled(
                180, 112, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
        thumbnail->setData(Qt::UserRole, photo.absoluteFilePath());
        thumbnail->setTextAlignment(Qt::AlignCenter);
        ui->recordsTable->setItem(row, 0, thumbnail);

        auto *name = new QTableWidgetItem(photo.fileName());
        name->setData(Qt::UserRole, photo.absoluteFilePath());
        name->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        ui->recordsTable->setItem(row, 1, name);

        auto *time = new QTableWidgetItem(
            photo.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        time->setData(Qt::UserRole, photo.absoluteFilePath());
        time->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        ui->recordsTable->setItem(row, 2, time);

        auto *size = new QTableWidgetItem(
            QStringLiteral("%1 KB").arg(photo.size() / 1024.0, 0, 'f', 1));
        size->setData(Qt::UserRole, photo.absoluteFilePath());
        size->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        ui->recordsTable->setItem(row, 3, size);
        ui->recordsTable->setRowHeight(row, 136);
    }

    if (photos.isEmpty()) {
        ui->recordsTable->setRowCount(1);
        auto *empty = new QTableWidgetItem(
            QStringLiteral("暂无历史照片，请先拍照保存"));
        empty->setTextAlignment(Qt::AlignCenter);
        ui->recordsTable->setItem(0, 0, empty);
        ui->recordsTable->setSpan(0, 0, 1, 4);
    }
}

void MainWindow::openHistoryPhoto(int row, int column)
{
    QTableWidgetItem *item = ui->recordsTable->item(row, column);
    if (item == nullptr) {
        return;
    }

    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty()) {
        return;
    }

    const QPixmap pixmap(path);
    if (pixmap.isNull()) {
        QMessageBox::warning(this,
                             QStringLiteral("无法查看照片"),
                             QStringLiteral("照片文件无法读取：%1").arg(path));
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QFileInfo(path).fileName());
    dialog.resize(960, 680);
    auto *layout = new QVBoxLayout(&dialog);
    auto *imageLabel = new QLabel(&dialog);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setPixmap(pixmap.scaled(
        900, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(imageLabel);
    dialog.exec();
}

void MainWindow::updateDetectionButton()
{
    ui->detectButton->setText(m_detecting
        ? QStringLiteral("结束检测")
        : QStringLiteral("开始检测"));
    ui->detectButton->setProperty("running", m_detecting);
    ui->detectButton->style()->unpolish(ui->detectButton);
    ui->detectButton->style()->polish(ui->detectButton);

    ui->runStatusLabel->setText(m_detecting
        ? QStringLiteral("正在检测")
        : QStringLiteral("等待开始"));
    ui->runStatusDot->setProperty("state", m_detecting ? "running" : "idle");
    ui->runStatusDot->style()->unpolish(ui->runStatusDot);
    ui->runStatusDot->style()->polish(ui->runStatusDot);
}

void MainWindow::updatePreviewPixmap()
{
    if (m_lastFrame.isNull()) {
        return;
    }

    const QPixmap pixmap = QPixmap::fromImage(m_lastFrame).scaled(
        ui->previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->previewLabel->setPixmap(pixmap);
    ui->previewHintLabel->hide();
    ui->previewSubHintLabel->hide();
}
