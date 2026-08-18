#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QStyle>
#include <QTimer>

namespace {

QString formatFileSize(qint64 bytes)
{
    constexpr double bytesPerKilobyte = 1024.0;
    constexpr double bytesPerMegabyte = 1024.0 * 1024.0;
    if (bytes >= static_cast<qint64>(bytesPerMegabyte)) {
        return QStringLiteral("%1 MB").arg(bytes / bytesPerMegabyte, 0, 'f', 1);
    }
    return QStringLiteral("%1 KB").arg(bytes / bytesPerKilobyte, 0, 'f', 1);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_clockTimer(new QTimer(this))
    , m_storageTimer(new QTimer(this))
    , m_storagePath(QDir::homePath() + QStringLiteral("/InsulatorMonitor/data"))
    , m_photoArchive(m_storagePath)
    , m_detecting(false)
    , m_detailScale(1.0)
    , m_detailFitMode(true)
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
            this, &MainWindow::openPhotoDetail);
    connect(ui->backToRecordsButton, &QPushButton::clicked,
            this, &MainWindow::backToRecordsPage);
    connect(ui->zoomInButton, &QPushButton::clicked,
            this, &MainWindow::zoomDetailIn);
    connect(ui->zoomOutButton, &QPushButton::clicked,
            this, &MainWindow::zoomDetailOut);
    connect(ui->zoomResetButton, &QPushButton::clicked,
            this, &MainWindow::resetDetailZoom);
    connect(ui->fitImageButton, &QPushButton::clicked,
            this, &MainWindow::fitDetailImage);
    connect(ui->exportPhotoButton, &QPushButton::clicked,
            this, &MainWindow::exportCurrentPhoto);
    connect(ui->deletePhotoButton, &QPushButton::clicked,
            this, &MainWindow::deleteCurrentPhoto);
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
    clearDetailState();
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
    if (ui->pageStack->currentWidget() == ui->photoDetailPage
            && m_detailFitMode && !m_detailPixmap.isNull()) {
        updateDetailPixmap();
    }
}

void MainWindow::setDetectionResult(const QString &result,
                                    double confidence,
                                    qint64 totalPixels,
                                    qint64 defectPixels)
{
    // Compatibility slot retained for DesktopUiPreview and older callers.
    // The merged UI intentionally removed the old result card, so there is
    // no longer a truthful widget target for these values.
    Q_UNUSED(result);
    Q_UNUSED(confidence);
    Q_UNUSED(totalPixels);
    Q_UNUSED(defectPixels);
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

void MainWindow::backToRecordsPage()
{
    refreshHistoryPhotos();
    ui->pageStack->setCurrentWidget(ui->recordsPage);
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
    const QVector<PhotoRecord> photos = m_photoArchive.records();

    ui->recordsTable->clearSpans();
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
    ui->recordsCountLabel->setText(
        QStringLiteral("共 %1 张").arg(photos.size()));

    for (int row = 0; row < photos.size(); ++row) {
        const PhotoRecord &photo = photos.at(row);
        auto *thumbnail = new QTableWidgetItem;
        const QPixmap pixmap(photo.path);
        if (!pixmap.isNull()) {
            thumbnail->setIcon(QIcon(pixmap.scaled(
                180, 112, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
        thumbnail->setData(Qt::UserRole, photo.path);
        thumbnail->setTextAlignment(Qt::AlignCenter);
        ui->recordsTable->setItem(row, 0, thumbnail);

        auto *name = new QTableWidgetItem(photo.fileName);
        name->setData(Qt::UserRole, photo.path);
        name->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        ui->recordsTable->setItem(row, 1, name);

        auto *time = new QTableWidgetItem(
            photo.modified.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        time->setData(Qt::UserRole, photo.path);
        time->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        ui->recordsTable->setItem(row, 2, time);

        auto *size = new QTableWidgetItem(formatFileSize(photo.bytes));
        size->setData(Qt::UserRole, photo.path);
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

void MainWindow::openPhotoDetail(int row, int column)
{
    QTableWidgetItem *item = ui->recordsTable->item(row, column);
    if (item == nullptr) {
        return;
    }

    const QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty()) {
        return;
    }

    const QFileInfo fileInfo(path);
    const QPixmap pixmap(path);
    if (!fileInfo.exists() || !fileInfo.isReadable() || pixmap.isNull()) {
        QMessageBox::warning(this,
                             QStringLiteral("无法查看照片"),
                             QStringLiteral("照片文件无法读取：%1").arg(path));
        return;
    }

    m_currentPhotoPath = fileInfo.absoluteFilePath();
    m_detailPixmap = pixmap;
    m_detailScale = 1.0;
    m_detailFitMode = true;

    ui->detailFileNameLabel->setText(fileInfo.fileName());
    ui->detailFileSizeValueLabel->setText(formatFileSize(fileInfo.size()));
    ui->detailImageSizeValueLabel->setText(
        QStringLiteral("%1 × %2")
            .arg(m_detailPixmap.width())
            .arg(m_detailPixmap.height()));
    ui->detailPathValueLabel->setText(m_currentPhotoPath);
    ui->detailResultValueLabel->setText(QStringLiteral("未记录"));

    ui->pageStack->setCurrentWidget(ui->photoDetailPage);
    updateDetailPixmap();
    QTimer::singleShot(0, this, [this] {
        if (ui->pageStack->currentWidget() == ui->photoDetailPage) {
            updateDetailPixmap();
        }
    });
}

double MainWindow::detailFitScale() const
{
    if (m_detailPixmap.isNull()) {
        return 1.0;
    }

    const QSize viewportSize = ui->detailImageScrollArea->viewport()->size();
    if (viewportSize.width() <= 0 || viewportSize.height() <= 0) {
        return 1.0;
    }

    const double widthScale = static_cast<double>(viewportSize.width())
        / m_detailPixmap.width();
    const double heightScale = static_cast<double>(viewportSize.height())
        / m_detailPixmap.height();
    return qMin(widthScale, heightScale);
}

void MainWindow::updateDetailPixmap()
{
    if (m_detailPixmap.isNull()) {
        return;
    }

    double displayScale = m_detailScale;
    if (m_detailFitMode) {
        displayScale = detailFitScale();
        ui->detailImageLabel->setMinimumSize(0, 0);
        ui->zoomValueLabel->setText(QStringLiteral("适应"));
    } else {
        const QSize scaledSize(
            qMax(1, qRound(m_detailPixmap.width() * displayScale)),
            qMax(1, qRound(m_detailPixmap.height() * displayScale)));
        ui->detailImageLabel->setMinimumSize(scaledSize);
        ui->zoomValueLabel->setText(
            QStringLiteral("%1%").arg(qRound(displayScale * 100.0)));
    }

    const QSize targetSize(
        qMax(1, qRound(m_detailPixmap.width() * displayScale)),
        qMax(1, qRound(m_detailPixmap.height() * displayScale)));
    ui->detailImageLabel->setText(QString());
    ui->detailImageLabel->setPixmap(m_detailPixmap.scaled(
        targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->detailImageLabel->updateGeometry();

    ui->zoomOutButton->setEnabled(displayScale > 0.25);
    ui->zoomInButton->setEnabled(displayScale < 4.0);
    ui->zoomResetButton->setEnabled(true);
    ui->fitImageButton->setEnabled(true);
    ui->exportPhotoButton->setEnabled(!m_currentPhotoPath.isEmpty());
    ui->deletePhotoButton->setEnabled(!m_currentPhotoPath.isEmpty());
}

void MainWindow::zoomDetailIn()
{
    if (m_detailPixmap.isNull()) {
        return;
    }

    const double currentScale = m_detailFitMode ? detailFitScale() : m_detailScale;
    m_detailFitMode = false;
    m_detailScale = qBound(0.25, currentScale + 0.25, 4.0);
    updateDetailPixmap();
}

void MainWindow::zoomDetailOut()
{
    if (m_detailPixmap.isNull()) {
        return;
    }

    const double currentScale = m_detailFitMode ? detailFitScale() : m_detailScale;
    if (currentScale <= 0.25) {
        return;
    }
    m_detailFitMode = false;
    m_detailScale = qBound(0.25, currentScale - 0.25, 4.0);
    updateDetailPixmap();
}

void MainWindow::resetDetailZoom()
{
    if (m_detailPixmap.isNull()) {
        return;
    }
    m_detailFitMode = false;
    m_detailScale = 1.0;
    updateDetailPixmap();
}

void MainWindow::fitDetailImage()
{
    if (m_detailPixmap.isNull()) {
        return;
    }
    m_detailFitMode = true;
    updateDetailPixmap();
}

void MainWindow::exportCurrentPhoto()
{
    if (m_currentPhotoPath.isEmpty() || m_detailPixmap.isNull()) {
        return;
    }

    QString exportDirectory = QStandardPaths::writableLocation(
        QStandardPaths::DocumentsLocation);
    if (exportDirectory.isEmpty()) {
        exportDirectory = QDir::homePath();
    }
    const QString defaultPath = QDir(exportDirectory).filePath(
        QFileInfo(m_currentPhotoPath).fileName());
    const QString destinationPath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出照片"),
        defaultPath,
        QStringLiteral("照片 (*.jpg *.jpeg *.png);;所有文件 (*)"));
    if (destinationPath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!m_photoArchive.exportPhoto(
            m_currentPhotoPath, destinationPath, &errorMessage)) {
        QMessageBox::critical(this,
                              QStringLiteral("导出失败"),
                              errorMessage);
        return;
    }

    QMessageBox::information(
        this,
        QStringLiteral("导出成功"),
        QStringLiteral("照片已导出到：%1").arg(destinationPath));
}

void MainWindow::deleteCurrentPhoto()
{
    if (m_currentPhotoPath.isEmpty() || m_detailPixmap.isNull()) {
        return;
    }

    const QString fileName = QFileInfo(m_currentPhotoPath).fileName();
    if (QMessageBox::question(
            this,
            QStringLiteral("删除照片"),
            QStringLiteral("确定要删除“%1”吗？此操作无法撤销。").arg(fileName),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    QString errorMessage;
    if (!m_photoArchive.remove(m_currentPhotoPath, &errorMessage)) {
        QMessageBox::critical(this,
                              QStringLiteral("删除失败"),
                              errorMessage);
        return;
    }

    clearDetailState();
    refreshHistoryPhotos();
    updateStorageSpace();
    ui->pageStack->setCurrentWidget(ui->recordsPage);
}

void MainWindow::clearDetailState()
{
    m_currentPhotoPath.clear();
    m_detailPixmap = QPixmap();
    m_detailScale = 1.0;
    m_detailFitMode = true;

    ui->detailFileNameLabel->setText(QStringLiteral("--"));
    ui->detailFileSizeValueLabel->setText(QStringLiteral("--"));
    ui->detailImageSizeValueLabel->setText(QStringLiteral("--"));
    ui->detailPathValueLabel->setText(QStringLiteral("--"));
    ui->detailResultValueLabel->setText(QStringLiteral("未记录"));
    ui->zoomValueLabel->setText(QStringLiteral("100%"));
    ui->detailImageLabel->setMinimumSize(0, 0);
    ui->detailImageLabel->setPixmap(QPixmap());
    ui->detailImageLabel->setText(
        QStringLiteral("选择历史照片后显示大图"));
    ui->zoomInButton->setEnabled(false);
    ui->zoomOutButton->setEnabled(false);
    ui->zoomResetButton->setEnabled(false);
    ui->fitImageButton->setEnabled(false);
    ui->exportPhotoButton->setEnabled(false);
    ui->deletePhotoButton->setEnabled(false);
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
