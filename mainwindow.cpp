#include "mainwindow.h"
#include "quantificationservice.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QIcon>
#include <QMessageBox>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QThread>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>

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

QString recordRootPath()
{
#ifdef Q_OS_WIN
    return QDir::homePath() + QStringLiteral("/InsulatorMonitor/data");
#else
    return QStringLiteral("/data/records");
#endif
}

QString resultSummary(const QJsonObject& result)
{
    const QString level = result.value(QStringLiteral("overall_level")).toString();
    const QString recommendation = result.value(QStringLiteral("recommendation")).toString();
    QStringList lines;
    if (!level.isEmpty()) {
        lines.append(level);
    }
    if (!recommendation.isEmpty()) {
        lines.append(recommendation);
    }
    if (result.contains(QStringLiteral("disc_count"))) {
        lines.append(QStringLiteral("绝缘子片：%1，缺陷实例：%2")
            .arg(result.value(QStringLiteral("disc_count")).toInt())
            .arg(result.value(QStringLiteral("defect_instance_count")).toInt()));
    }
    if (result.contains(QStringLiteral("highest_risk_disc_id"))) {
        lines.append(QStringLiteral("最高风险片：%1")
            .arg(result.value(QStringLiteral("highest_risk_disc_id")).toInt()));
    }
    if (lines.isEmpty()) {
        return QStringLiteral("量化完成");
    }
    return lines.join(QStringLiteral("\n"));
}

QString formattedJsonFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QStringLiteral("无法读取：%1").arg(path);
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    return error.error == QJsonParseError::NoError
        ? QString::fromUtf8(document.toJson(QJsonDocument::Indented))
        : QStringLiteral("JSON 无效：%1").arg(error.errorString());
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_clockTimer(new QTimer(this))
    , m_storageTimer(new QTimer(this))
    , m_storagePath(recordRootPath())
    , m_photoArchive(m_storagePath)
    , m_quantificationThread(new QThread(this))
    , m_quantificationService(new QuantificationService(m_storagePath))
    , m_detecting(false)
    , m_snapshotPending(false)
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
    connect(ui->clearRecordsButton, &QPushButton::clicked,
            this, &MainWindow::clearAllPhotos);
    connect(ui->backButton, &QPushButton::clicked,
            this, &MainWindow::openMonitorPage);
    connect(ui->settingsButton, &QPushButton::clicked,
            this, &MainWindow::openSettingsPage);
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
    connect(ui->viewJsonButton, &QPushButton::clicked,
            this, &MainWindow::viewCurrentJson);
    connect(ui->quantifyPhotoButton, &QPushButton::clicked,
            this, &MainWindow::quantifyCurrentPhoto);
    connect(ui->exportPhotoButton, &QPushButton::clicked,
            this, &MainWindow::exportCurrentPhoto);
    connect(ui->deletePhotoButton, &QPushButton::clicked,
            this, &MainWindow::deleteCurrentPhoto);
    connect(ui->settingBackButton, &QPushButton::clicked,
            this, &MainWindow::openMonitorPage);
    m_quantificationService->moveToThread(m_quantificationThread);
    connect(this, &MainWindow::quantificationRequested,
            m_quantificationService, &QuantificationService::process);
    connect(m_quantificationService, &QuantificationService::completed,
            this, &MainWindow::handleQuantificationCompleted);
    connect(m_quantificationService, &QuantificationService::failed,
            this, &MainWindow::handleQuantificationFailed);
    connect(m_quantificationThread, &QThread::finished,
            m_quantificationService, &QObject::deleteLater);
    m_quantificationThread->start();
    //connect(  发送者,        &发送者类名::信号,      接收者,       &接收者类名::槽函数 );
    m_clockTimer->start(1000);
    updateClock();
    QDir().mkpath(m_storagePath);
    connect(m_storageTimer, &QTimer::timeout,
            this, &MainWindow::updateStorageSpace);
    m_storageTimer->start(5000);
    updateStorageSpace();
    updateDetectionButton();
    updateSnapshotButton();
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
    m_quantificationThread->quit();
    m_quantificationThread->wait();
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
    updateSnapshotButton();
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

void MainWindow::openSettingsPage()
{
    ui->pageStack->setCurrentWidget(ui->settingPage);
}

void MainWindow::openRecordsPage()//照片界面
{
    refreshHistoryPhotos();
    ui->pageStack->setCurrentWidget(ui->recordsPage);
}

void MainWindow::clearAllPhotos()
{
    if (m_photoArchive.records().isEmpty()) {
        return;
    }
    if (QMessageBox::question(
            this,
            QStringLiteral("清空记录"),
            QStringLiteral("确定要删除全部拍照记录吗？此操作无法撤销。"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    const PhotoClearResult result = m_photoArchive.clear();
    clearDetailState();
    refreshHistoryPhotos();
    updateStorageSpace();
    ui->snapshotCountValueLabel->setText(
        QString::number(m_photoArchive.records().size()));

    if (result.failed > 0) {
        QMessageBox::warning(
            this,
            QStringLiteral("清空记录未完成"),
            QStringLiteral("已删除 %1 条，%2 条删除失败。请检查文件是否被其他程序占用。")
                .arg(result.removed)
                .arg(result.failed));
    } else {
        ui->runStatusLabel->setText(QStringLiteral("拍照记录已清空"));
    }
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
    if (!m_detecting) {
        QMessageBox::warning(
            this,
            QStringLiteral("拍照失败"),
            QStringLiteral("请先启动 RKNN 检测后再拍照"));
        return;
    }
    if (m_snapshotPending) {
        return;
    }

    QDir().mkpath(m_storagePath);
    const QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_snapshotPending = true;
    m_pendingSnapshotId = requestId;
    updateSnapshotButton();
    emit snapshotRequested({requestId, QDir(m_storagePath).filePath(requestId)});
}

void MainWindow::processSnapshotPackage(const SnapshotPackage& package)
{
    const QString expectedRecordDir = QDir(m_storagePath).filePath(m_pendingSnapshotId);
    if (m_snapshotPending && package.requestId == m_pendingSnapshotId
        && QDir(package.recordDir).absolutePath() == QDir(expectedRecordDir).absolutePath()) {
        m_snapshotPending = false;
        m_pendingSnapshotId.clear();
        updateSnapshotButton();
        refreshHistoryPhotos();
        ui->snapshotCountValueLabel->setText(
            QString::number(m_photoArchive.records().size()));
        updateStorageSpace();
        ui->runStatusLabel->setText(QStringLiteral("照片、mask 和 metadata 已保存"));

        auto* toast = new QMessageBox(
            QMessageBox::Information, QString(), QStringLiteral("保存成功"),
            QMessageBox::NoButton, this);
        toast->setWindowFlag(Qt::FramelessWindowHint);
        toast->setAttribute(Qt::WA_DeleteOnClose);
        toast->show();
        toast->move(geometry().center() - toast->rect().center());
        QTimer::singleShot(1000, toast, &QWidget::close);
    }
}

void MainWindow::handleSnapshotFailure(const QString& requestId, const QString& message)
{
    if (!m_snapshotPending || requestId != m_pendingSnapshotId) {
        return;
    }
    m_snapshotPending = false;
    m_pendingSnapshotId.clear();
    updateSnapshotButton();
    QMessageBox::warning(this, QStringLiteral("拍照失败"), message);
}

void MainWindow::handleQuantificationCompleted(const SnapshotPackage& package,
                                                const QJsonObject& result)
{
    Q_UNUSED(package)
    m_snapshotPending = false;
    updateSnapshotButton();
    ui->detailResultValueLabel->setText(resultSummary(result));
    ui->runStatusLabel->setText(resultSummary(result));
    ui->snapshotCountValueLabel->setText(QString::number(m_photoArchive.records().size()));
    refreshHistoryPhotos();
    updateStorageSpace();
}

void MainWindow::handleQuantificationFailed(const SnapshotPackage& package,
                                             const QString& message)
{
    Q_UNUSED(package)
    m_snapshotPending = false;
    updateSnapshotButton();
    refreshHistoryPhotos();
    QMessageBox::warning(this, QStringLiteral("量化未完成"),
                         QStringLiteral("照片、mask 和 metadata 已保留。\n%1").arg(message));
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
        QStringLiteral("共 %1 条").arg(photos.size()));

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
            QStringLiteral("暂无拍照记录，请先拍照保存"));
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
    QFile resultFile(fileInfo.absoluteDir().filePath(QStringLiteral("result.json")));
    QJsonParseError parseError;
    if (resultFile.open(QIODevice::ReadOnly)) {
        const QJsonDocument resultDocument = QJsonDocument::fromJson(resultFile.readAll(), &parseError);
        ui->detailResultValueLabel->setText(
            parseError.error == QJsonParseError::NoError && resultDocument.isObject()
                ? resultSummary(resultDocument.object()) : QStringLiteral("结果文件无效"));
    } else {
        ui->detailResultValueLabel->setText(QStringLiteral("尚未量化"));
    }

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
    ui->viewJsonButton->setEnabled(!m_currentPhotoPath.isEmpty());
    ui->quantifyPhotoButton->setEnabled(
        QFileInfo(m_currentPhotoPath).fileName() == QStringLiteral("image.jpg"));
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
    const QString destinationPath = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("选择导出目录"),
        exportDirectory);
    if (destinationPath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!m_photoArchive.exportRecord(
            m_currentPhotoPath, destinationPath, &errorMessage)) {
        QMessageBox::critical(this,
                              QStringLiteral("导出失败"),
                              errorMessage);
        return;
    }

    QMessageBox::information(
        this,
        QStringLiteral("导出成功"),
            QStringLiteral("完整记录已导出到：%1").arg(destinationPath));
}

void MainWindow::deleteCurrentPhoto()
{
    if (m_currentPhotoPath.isEmpty() || m_detailPixmap.isNull()) {
        return;
    }

    const QString fileName = QFileInfo(m_currentPhotoPath).absoluteDir().dirName();
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
    ui->viewJsonButton->setEnabled(false);
    ui->quantifyPhotoButton->setEnabled(false);
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

void MainWindow::viewCurrentJson()
{
    if (m_currentPhotoPath.isEmpty()) {
        return;
    }

    const QDir recordDir(QFileInfo(m_currentPhotoPath).absoluteDir());
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("记录 JSON"));
    dialog.resize(860, 620);
    auto* layout = new QVBoxLayout(&dialog);
    auto* viewer = new QPlainTextEdit(&dialog);
    viewer->setReadOnly(true);
    viewer->setLineWrapMode(QPlainTextEdit::NoWrap);
    viewer->setPlainText(
        QStringLiteral("metadata.json\n%1\n\nresult.json\n%2")
            .arg(formattedJsonFile(recordDir.filePath(QStringLiteral("metadata.json"))),
                 formattedJsonFile(recordDir.filePath(QStringLiteral("result.json")))));
    layout->addWidget(viewer);
    auto* closeButton = new QPushButton(QStringLiteral("关闭"), &dialog);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(closeButton);
    dialog.exec();
}

void MainWindow::quantifyCurrentPhoto()
{
    const QFileInfo imageInfo(m_currentPhotoPath);
    if (imageInfo.fileName() != QStringLiteral("image.jpg")) {
        QMessageBox::information(this, QStringLiteral("无法量化"),
                                 QStringLiteral("旧照片没有对应的 mask 和 metadata"));
        return;
    }
    const QDir recordDir(imageInfo.absoluteDir());
    ui->runStatusLabel->setText(QStringLiteral("正在量化当前照片"));
    emit quantificationRequested({recordDir.dirName(), recordDir.absolutePath()});
}

void MainWindow::updateSnapshotButton()
{
    ui->snapshotButton->setEnabled(m_detecting && !m_snapshotPending);
    ui->snapshotButton->setText(m_snapshotPending
        ? QStringLiteral("保存中…") : QStringLiteral("拍照保存"));
}

void MainWindow::moveFailedRecord(const SnapshotPackage& package)
{
    const QFileInfo recordInfo(package.recordDir);
    if (recordInfo.absoluteDir().absolutePath() != m_storagePath || !recordInfo.exists()) {
        return;
    }
    const QDir failedRoot(QDir(m_storagePath).filePath(QStringLiteral(".failed")));
    QDir().mkpath(failedRoot.absolutePath());
    const QString destination = failedRoot.filePath(recordInfo.fileName());
    if (!QFileInfo::exists(destination)) {
        QDir(m_storagePath).rename(recordInfo.fileName(),
                                   QDir(QStringLiteral(".failed")).filePath(recordInfo.fileName()));
    }
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
