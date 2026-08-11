/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.8
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QVBoxLayout *rootLayout;
    QFrame *headerFrame;
    QHBoxLayout *headerLayout;
    QVBoxLayout *titleLayout;
    QLabel *appTitleLabel;
    QLabel *appSubTitleLabel;
    QSpacerItem *headerSpacer;
    QVBoxLayout *clockLayout;
    QLabel *timeLabel;
    QLabel *dateLabel;
    QSpacerItem *clockGap;
    QHBoxLayout *batteryLayout;
    QProgressBar *batteryProgress;
    QLabel *batteryValueLabel;
    QStackedWidget *pageStack;
    QWidget *monitorPage;
    QHBoxLayout *monitorLayout;
    QFrame *previewCard;
    QVBoxLayout *previewCardLayout;
    QHBoxLayout *previewTitleLayout;
    QLabel *previewTitleLabel;
    QSpacerItem *previewTitleSpacer;
    QLabel *runStatusDot;
    QLabel *runStatusLabel;
    QWidget *previewContainer;
    QGridLayout *previewGridLayout;
    QLabel *previewLabel;
    QVBoxLayout *previewHintLayout;
    QSpacerItem *previewHintTopSpacer;
    QLabel *previewHintLabel;
    QLabel *previewSubHintLabel;
    QSpacerItem *previewHintBottomSpacer;
    QVBoxLayout *sidePanelLayout;
    QFrame *resultCard;
    QVBoxLayout *resultCardLayout;
    QLabel *resultTitleLabel;
    QLabel *resultValueLabel;
    QFrame *resultLine;
    QGridLayout *metricsLayout;
    QLabel *confidenceNameLabel;
    QLabel *confidenceValueLabel;
    QLabel *totalPixelsNameLabel;
    QLabel *totalPixelsValueLabel;
    QLabel *defectPixelsNameLabel;
    QLabel *defectPixelsValueLabel;
    QLabel *defectRatioNameLabel;
    QLabel *defectRatioValueLabel;
    QFrame *actionCard;
    QVBoxLayout *actionCardLayout;
    QPushButton *detectButton;
    QPushButton *snapshotButton;
    QHBoxLayout *secondaryActionsLayout;
    QPushButton *recordsButton;
    QPushButton *settingsButton;
    QSpacerItem *sidePanelSpacer;
    QWidget *recordsPage;
    QVBoxLayout *recordsPageLayout;
    QHBoxLayout *recordsHeaderLayout;
    QPushButton *backButton;
    QLabel *recordsTitleLabel;
    QSpacerItem *recordsHeaderSpacer;
    QLabel *recordsHintLabel;
    QTableWidget *recordsTable;
    QFrame *footerFrame;
    QHBoxLayout *footerLayout;
    QLabel *deviceStatusDot;
    QLabel *deviceStatusLabel;
    QSpacerItem *footerGap1;
    QLabel *storageNameLabel;
    QLabel *storageValueLabel;
    QSpacerItem *footerGap2;
    QLabel *snapshotCountNameLabel;
    QLabel *snapshotCountValueLabel;
    QSpacerItem *footerMainSpacer;
    QPushButton *recordsNavButton;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(1280, 800);
        MainWindow->setMinimumSize(QSize(1024, 600));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        rootLayout = new QVBoxLayout(centralWidget);
        rootLayout->setSpacing(0);
        rootLayout->setObjectName(QString::fromUtf8("rootLayout"));
        rootLayout->setContentsMargins(0, 0, 0, 0);
        headerFrame = new QFrame(centralWidget);
        headerFrame->setObjectName(QString::fromUtf8("headerFrame"));
        headerFrame->setMinimumSize(QSize(0, 72));
        headerFrame->setMaximumSize(QSize(16777215, 72));
        headerLayout = new QHBoxLayout(headerFrame);
        headerLayout->setSpacing(14);
        headerLayout->setObjectName(QString::fromUtf8("headerLayout"));
        headerLayout->setContentsMargins(24, -1, 24, -1);
        titleLayout = new QVBoxLayout();
        titleLayout->setSpacing(1);
        titleLayout->setObjectName(QString::fromUtf8("titleLayout"));
        appTitleLabel = new QLabel(headerFrame);
        appTitleLabel->setObjectName(QString::fromUtf8("appTitleLabel"));

        titleLayout->addWidget(appTitleLabel);

        appSubTitleLabel = new QLabel(headerFrame);
        appSubTitleLabel->setObjectName(QString::fromUtf8("appSubTitleLabel"));

        titleLayout->addWidget(appSubTitleLabel);


        headerLayout->addLayout(titleLayout);

        headerSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        headerLayout->addItem(headerSpacer);

        clockLayout = new QVBoxLayout();
        clockLayout->setSpacing(0);
        clockLayout->setObjectName(QString::fromUtf8("clockLayout"));
        timeLabel = new QLabel(headerFrame);
        timeLabel->setObjectName(QString::fromUtf8("timeLabel"));
        timeLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        clockLayout->addWidget(timeLabel);

        dateLabel = new QLabel(headerFrame);
        dateLabel->setObjectName(QString::fromUtf8("dateLabel"));
        dateLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        clockLayout->addWidget(dateLabel);


        headerLayout->addLayout(clockLayout);

        clockGap = new QSpacerItem(12, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        headerLayout->addItem(clockGap);

        batteryLayout = new QHBoxLayout();
        batteryLayout->setSpacing(8);
        batteryLayout->setObjectName(QString::fromUtf8("batteryLayout"));
        batteryProgress = new QProgressBar(headerFrame);
        batteryProgress->setObjectName(QString::fromUtf8("batteryProgress"));
        batteryProgress->setValue(86);
        batteryProgress->setTextVisible(false);
        batteryProgress->setOrientation(Qt::Horizontal);

        batteryLayout->addWidget(batteryProgress);

        batteryValueLabel = new QLabel(headerFrame);
        batteryValueLabel->setObjectName(QString::fromUtf8("batteryValueLabel"));

        batteryLayout->addWidget(batteryValueLabel);


        headerLayout->addLayout(batteryLayout);


        rootLayout->addWidget(headerFrame);

        pageStack = new QStackedWidget(centralWidget);
        pageStack->setObjectName(QString::fromUtf8("pageStack"));
        monitorPage = new QWidget();
        monitorPage->setObjectName(QString::fromUtf8("monitorPage"));
        monitorLayout = new QHBoxLayout(monitorPage);
        monitorLayout->setSpacing(16);
        monitorLayout->setObjectName(QString::fromUtf8("monitorLayout"));
        monitorLayout->setContentsMargins(18, 16, 18, 16);
        previewCard = new QFrame(monitorPage);
        previewCard->setObjectName(QString::fromUtf8("previewCard"));
        previewCard->setProperty("card", QVariant(true));
        previewCardLayout = new QVBoxLayout(previewCard);
        previewCardLayout->setSpacing(10);
        previewCardLayout->setObjectName(QString::fromUtf8("previewCardLayout"));
        previewCardLayout->setContentsMargins(12, 12, 12, 12);
        previewTitleLayout = new QHBoxLayout();
        previewTitleLayout->setObjectName(QString::fromUtf8("previewTitleLayout"));
        previewTitleLabel = new QLabel(previewCard);
        previewTitleLabel->setObjectName(QString::fromUtf8("previewTitleLabel"));

        previewTitleLayout->addWidget(previewTitleLabel);

        previewTitleSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        previewTitleLayout->addItem(previewTitleSpacer);

        runStatusDot = new QLabel(previewCard);
        runStatusDot->setObjectName(QString::fromUtf8("runStatusDot"));

        previewTitleLayout->addWidget(runStatusDot);

        runStatusLabel = new QLabel(previewCard);
        runStatusLabel->setObjectName(QString::fromUtf8("runStatusLabel"));

        previewTitleLayout->addWidget(runStatusLabel);


        previewCardLayout->addLayout(previewTitleLayout);

        previewContainer = new QWidget(previewCard);
        previewContainer->setObjectName(QString::fromUtf8("previewContainer"));
        previewGridLayout = new QGridLayout(previewContainer);
        previewGridLayout->setObjectName(QString::fromUtf8("previewGridLayout"));
        previewGridLayout->setContentsMargins(0, 0, 0, 0);
        previewLabel = new QLabel(previewContainer);
        previewLabel->setObjectName(QString::fromUtf8("previewLabel"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(previewLabel->sizePolicy().hasHeightForWidth());
        previewLabel->setSizePolicy(sizePolicy);
        previewLabel->setMinimumSize(QSize(0, 360));
        previewLabel->setAlignment(Qt::AlignCenter);

        previewGridLayout->addWidget(previewLabel, 0, 0, 1, 1);

        previewHintLayout = new QVBoxLayout();
        previewHintLayout->setObjectName(QString::fromUtf8("previewHintLayout"));
        previewHintTopSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        previewHintLayout->addItem(previewHintTopSpacer);

        previewHintLabel = new QLabel(previewContainer);
        previewHintLabel->setObjectName(QString::fromUtf8("previewHintLabel"));
        previewHintLabel->setAlignment(Qt::AlignCenter);

        previewHintLayout->addWidget(previewHintLabel);

        previewSubHintLabel = new QLabel(previewContainer);
        previewSubHintLabel->setObjectName(QString::fromUtf8("previewSubHintLabel"));
        previewSubHintLabel->setAlignment(Qt::AlignCenter);

        previewHintLayout->addWidget(previewSubHintLabel);

        previewHintBottomSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        previewHintLayout->addItem(previewHintBottomSpacer);


        previewGridLayout->addLayout(previewHintLayout, 0, 0, 1, 1);


        previewCardLayout->addWidget(previewContainer);


        monitorLayout->addWidget(previewCard);

        sidePanelLayout = new QVBoxLayout();
        sidePanelLayout->setSpacing(12);
        sidePanelLayout->setObjectName(QString::fromUtf8("sidePanelLayout"));
        resultCard = new QFrame(monitorPage);
        resultCard->setObjectName(QString::fromUtf8("resultCard"));
        resultCard->setProperty("card", QVariant(true));
        resultCardLayout = new QVBoxLayout(resultCard);
        resultCardLayout->setSpacing(10);
        resultCardLayout->setObjectName(QString::fromUtf8("resultCardLayout"));
        resultCardLayout->setContentsMargins(18, 16, 18, 16);
        resultTitleLabel = new QLabel(resultCard);
        resultTitleLabel->setObjectName(QString::fromUtf8("resultTitleLabel"));

        resultCardLayout->addWidget(resultTitleLabel);

        resultValueLabel = new QLabel(resultCard);
        resultValueLabel->setObjectName(QString::fromUtf8("resultValueLabel"));
        resultValueLabel->setAlignment(Qt::AlignCenter);

        resultCardLayout->addWidget(resultValueLabel);

        resultLine = new QFrame(resultCard);
        resultLine->setObjectName(QString::fromUtf8("resultLine"));
        resultLine->setFrameShape(QFrame::HLine);
        resultLine->setFrameShadow(QFrame::Sunken);

        resultCardLayout->addWidget(resultLine);

        metricsLayout = new QGridLayout();
        metricsLayout->setObjectName(QString::fromUtf8("metricsLayout"));
        metricsLayout->setHorizontalSpacing(10);
        metricsLayout->setVerticalSpacing(9);
        confidenceNameLabel = new QLabel(resultCard);
        confidenceNameLabel->setObjectName(QString::fromUtf8("confidenceNameLabel"));

        metricsLayout->addWidget(confidenceNameLabel, 0, 0, 1, 1);

        confidenceValueLabel = new QLabel(resultCard);
        confidenceValueLabel->setObjectName(QString::fromUtf8("confidenceValueLabel"));
        confidenceValueLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        metricsLayout->addWidget(confidenceValueLabel, 0, 1, 1, 1);

        totalPixelsNameLabel = new QLabel(resultCard);
        totalPixelsNameLabel->setObjectName(QString::fromUtf8("totalPixelsNameLabel"));

        metricsLayout->addWidget(totalPixelsNameLabel, 1, 0, 1, 1);

        totalPixelsValueLabel = new QLabel(resultCard);
        totalPixelsValueLabel->setObjectName(QString::fromUtf8("totalPixelsValueLabel"));
        totalPixelsValueLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        metricsLayout->addWidget(totalPixelsValueLabel, 1, 1, 1, 1);

        defectPixelsNameLabel = new QLabel(resultCard);
        defectPixelsNameLabel->setObjectName(QString::fromUtf8("defectPixelsNameLabel"));

        metricsLayout->addWidget(defectPixelsNameLabel, 2, 0, 1, 1);

        defectPixelsValueLabel = new QLabel(resultCard);
        defectPixelsValueLabel->setObjectName(QString::fromUtf8("defectPixelsValueLabel"));
        defectPixelsValueLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        metricsLayout->addWidget(defectPixelsValueLabel, 2, 1, 1, 1);

        defectRatioNameLabel = new QLabel(resultCard);
        defectRatioNameLabel->setObjectName(QString::fromUtf8("defectRatioNameLabel"));

        metricsLayout->addWidget(defectRatioNameLabel, 3, 0, 1, 1);

        defectRatioValueLabel = new QLabel(resultCard);
        defectRatioValueLabel->setObjectName(QString::fromUtf8("defectRatioValueLabel"));
        defectRatioValueLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        metricsLayout->addWidget(defectRatioValueLabel, 3, 1, 1, 1);


        resultCardLayout->addLayout(metricsLayout);


        sidePanelLayout->addWidget(resultCard);

        actionCard = new QFrame(monitorPage);
        actionCard->setObjectName(QString::fromUtf8("actionCard"));
        actionCard->setProperty("card", QVariant(true));
        actionCardLayout = new QVBoxLayout(actionCard);
        actionCardLayout->setSpacing(10);
        actionCardLayout->setObjectName(QString::fromUtf8("actionCardLayout"));
        actionCardLayout->setContentsMargins(14, 14, 14, 14);
        detectButton = new QPushButton(actionCard);
        detectButton->setObjectName(QString::fromUtf8("detectButton"));
        detectButton->setProperty("running", QVariant(false));

        actionCardLayout->addWidget(detectButton);

        snapshotButton = new QPushButton(actionCard);
        snapshotButton->setObjectName(QString::fromUtf8("snapshotButton"));

        actionCardLayout->addWidget(snapshotButton);

        secondaryActionsLayout = new QHBoxLayout();
        secondaryActionsLayout->setSpacing(9);
        secondaryActionsLayout->setObjectName(QString::fromUtf8("secondaryActionsLayout"));
        recordsButton = new QPushButton(actionCard);
        recordsButton->setObjectName(QString::fromUtf8("recordsButton"));
        recordsButton->setProperty("compact", QVariant(true));

        secondaryActionsLayout->addWidget(recordsButton);

        settingsButton = new QPushButton(actionCard);
        settingsButton->setObjectName(QString::fromUtf8("settingsButton"));
        settingsButton->setProperty("compact", QVariant(true));

        secondaryActionsLayout->addWidget(settingsButton);


        actionCardLayout->addLayout(secondaryActionsLayout);


        sidePanelLayout->addWidget(actionCard);

        sidePanelSpacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);

        sidePanelLayout->addItem(sidePanelSpacer);


        monitorLayout->addLayout(sidePanelLayout);

        monitorLayout->setStretch(0, 7);
        monitorLayout->setStretch(1, 3);
        pageStack->addWidget(monitorPage);
        recordsPage = new QWidget();
        recordsPage->setObjectName(QString::fromUtf8("recordsPage"));
        recordsPageLayout = new QVBoxLayout(recordsPage);
        recordsPageLayout->setSpacing(14);
        recordsPageLayout->setObjectName(QString::fromUtf8("recordsPageLayout"));
        recordsPageLayout->setContentsMargins(20, 18, 20, 18);
        recordsHeaderLayout = new QHBoxLayout();
        recordsHeaderLayout->setObjectName(QString::fromUtf8("recordsHeaderLayout"));
        backButton = new QPushButton(recordsPage);
        backButton->setObjectName(QString::fromUtf8("backButton"));
        backButton->setProperty("compact", QVariant(true));

        recordsHeaderLayout->addWidget(backButton);

        recordsTitleLabel = new QLabel(recordsPage);
        recordsTitleLabel->setObjectName(QString::fromUtf8("recordsTitleLabel"));

        recordsHeaderLayout->addWidget(recordsTitleLabel);

        recordsHeaderSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        recordsHeaderLayout->addItem(recordsHeaderSpacer);

        recordsHintLabel = new QLabel(recordsPage);
        recordsHintLabel->setObjectName(QString::fromUtf8("recordsHintLabel"));

        recordsHeaderLayout->addWidget(recordsHintLabel);


        recordsPageLayout->addLayout(recordsHeaderLayout);

        recordsTable = new QTableWidget(recordsPage);
        if (recordsTable->columnCount() < 8)
            recordsTable->setColumnCount(8);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        recordsTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        recordsTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        recordsTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        recordsTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        recordsTable->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        recordsTable->setHorizontalHeaderItem(5, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        recordsTable->setHorizontalHeaderItem(6, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        recordsTable->setHorizontalHeaderItem(7, __qtablewidgetitem7);
        recordsTable->setObjectName(QString::fromUtf8("recordsTable"));
        recordsTable->setAlternatingRowColors(true);
        recordsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        recordsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        recordsTable->setShowGrid(true);
        recordsTable->setColumnCount(8);
        recordsTable->setRowCount(0);

        recordsPageLayout->addWidget(recordsTable);

        pageStack->addWidget(recordsPage);

        rootLayout->addWidget(pageStack);

        footerFrame = new QFrame(centralWidget);
        footerFrame->setObjectName(QString::fromUtf8("footerFrame"));
        footerFrame->setMinimumSize(QSize(0, 54));
        footerFrame->setMaximumSize(QSize(16777215, 54));
        footerLayout = new QHBoxLayout(footerFrame);
        footerLayout->setSpacing(9);
        footerLayout->setObjectName(QString::fromUtf8("footerLayout"));
        footerLayout->setContentsMargins(22, -1, 22, -1);
        deviceStatusDot = new QLabel(footerFrame);
        deviceStatusDot->setObjectName(QString::fromUtf8("deviceStatusDot"));

        footerLayout->addWidget(deviceStatusDot);

        deviceStatusLabel = new QLabel(footerFrame);
        deviceStatusLabel->setObjectName(QString::fromUtf8("deviceStatusLabel"));

        footerLayout->addWidget(deviceStatusLabel);

        footerGap1 = new QSpacerItem(18, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        footerLayout->addItem(footerGap1);

        storageNameLabel = new QLabel(footerFrame);
        storageNameLabel->setObjectName(QString::fromUtf8("storageNameLabel"));

        footerLayout->addWidget(storageNameLabel);

        storageValueLabel = new QLabel(footerFrame);
        storageValueLabel->setObjectName(QString::fromUtf8("storageValueLabel"));

        footerLayout->addWidget(storageValueLabel);

        footerGap2 = new QSpacerItem(18, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        footerLayout->addItem(footerGap2);

        snapshotCountNameLabel = new QLabel(footerFrame);
        snapshotCountNameLabel->setObjectName(QString::fromUtf8("snapshotCountNameLabel"));

        footerLayout->addWidget(snapshotCountNameLabel);

        snapshotCountValueLabel = new QLabel(footerFrame);
        snapshotCountValueLabel->setObjectName(QString::fromUtf8("snapshotCountValueLabel"));

        footerLayout->addWidget(snapshotCountValueLabel);

        footerMainSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        footerLayout->addItem(footerMainSpacer);

        recordsNavButton = new QPushButton(footerFrame);
        recordsNavButton->setObjectName(QString::fromUtf8("recordsNavButton"));
        recordsNavButton->setProperty("compact", QVariant(true));

        footerLayout->addWidget(recordsNavButton);


        rootLayout->addWidget(footerFrame);

        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        pageStack->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\347\273\235\347\274\230\345\255\220\346\231\272\350\203\275\346\243\200\346\265\213\347\263\273\347\273\237", nullptr));
        appTitleLabel->setText(QCoreApplication::translate("MainWindow", "\347\273\235\347\274\230\345\255\220\346\231\272\350\203\275\346\243\200\346\265\213\347\263\273\347\273\237", nullptr));
        appSubTitleLabel->setText(QCoreApplication::translate("MainWindow", "\347\247\273\345\212\250\345\274\217\350\247\206\350\247\211\345\267\241\346\243\200\347\273\210\347\253\257", nullptr));
        timeLabel->setText(QCoreApplication::translate("MainWindow", "00:00:00", nullptr));
        dateLabel->setText(QCoreApplication::translate("MainWindow", "2026\345\271\26407\346\234\21022\346\227\245", nullptr));
        batteryValueLabel->setText(QCoreApplication::translate("MainWindow", "86%", nullptr));
        batteryValueLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricValue", nullptr)));
        previewTitleLabel->setText(QCoreApplication::translate("MainWindow", "\345\256\236\346\227\266\346\243\200\346\265\213\347\224\273\351\235\242", nullptr));
        previewTitleLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "sectionTitle", nullptr)));
        runStatusDot->setProperty("state", QVariant(QCoreApplication::translate("MainWindow", "idle", nullptr)));
        runStatusDot->setText(QString());
        runStatusLabel->setText(QCoreApplication::translate("MainWindow", "\347\255\211\345\276\205\345\274\200\345\247\213", nullptr));
        runStatusLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "muted", nullptr)));
        previewLabel->setText(QString());
        previewHintLabel->setText(QCoreApplication::translate("MainWindow", "\347\255\211\345\276\205\346\221\204\345\203\217\345\244\264\347\224\273\351\235\242", nullptr));
        previewSubHintLabel->setText(QCoreApplication::translate("MainWindow", "\350\277\236\346\216\245\346\221\204\345\203\217\345\244\264\345\220\216\345\260\206\345\234\250\346\255\244\345\244\204\346\230\276\347\244\272\345\256\236\346\227\266\345\233\276\345\203\217", nullptr));
        resultTitleLabel->setText(QCoreApplication::translate("MainWindow", "\345\275\223\345\211\215\350\257\206\345\210\253\347\273\223\346\236\234", nullptr));
        resultTitleLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "sectionTitle", nullptr)));
        resultValueLabel->setText(QCoreApplication::translate("MainWindow", "\347\255\211\345\276\205\346\243\200\346\265\213", nullptr));
        confidenceNameLabel->setText(QCoreApplication::translate("MainWindow", "\347\275\256\344\277\241\345\272\246", nullptr));
        confidenceNameLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricName", nullptr)));
        confidenceValueLabel->setText(QCoreApplication::translate("MainWindow", "--", nullptr));
        confidenceValueLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricValue", nullptr)));
        totalPixelsNameLabel->setText(QCoreApplication::translate("MainWindow", "\347\273\235\347\274\230\345\255\220\346\200\273\345\203\217\347\264\240", nullptr));
        totalPixelsNameLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricName", nullptr)));
        totalPixelsValueLabel->setText(QCoreApplication::translate("MainWindow", "--", nullptr));
        totalPixelsValueLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricValue", nullptr)));
        defectPixelsNameLabel->setText(QCoreApplication::translate("MainWindow", "\347\274\272\351\231\267\345\203\217\347\264\240", nullptr));
        defectPixelsNameLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricName", nullptr)));
        defectPixelsValueLabel->setText(QCoreApplication::translate("MainWindow", "--", nullptr));
        defectPixelsValueLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricValue", nullptr)));
        defectRatioNameLabel->setText(QCoreApplication::translate("MainWindow", "\347\274\272\351\231\267\345\203\217\347\264\240\345\215\240\346\257\224", nullptr));
        defectRatioNameLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricName", nullptr)));
        defectRatioValueLabel->setText(QCoreApplication::translate("MainWindow", "--", nullptr));
        defectRatioValueLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricValue", nullptr)));
        detectButton->setText(QCoreApplication::translate("MainWindow", "\345\274\200\345\247\213\346\243\200\346\265\213", nullptr));
        snapshotButton->setText(QCoreApplication::translate("MainWindow", "\346\213\215\347\205\247\344\277\235\345\255\230", nullptr));
        recordsButton->setText(QCoreApplication::translate("MainWindow", "\346\243\200\346\265\213\350\256\260\345\275\225", nullptr));
        settingsButton->setText(QCoreApplication::translate("MainWindow", "\350\256\276\347\275\256", nullptr));
        backButton->setText(QCoreApplication::translate("MainWindow", "\350\277\224\345\233\236\345\256\236\346\227\266\346\243\200\346\265\213", nullptr));
        recordsTitleLabel->setText(QCoreApplication::translate("MainWindow", "\346\243\200\346\265\213\350\256\260\345\275\225", nullptr));
        recordsTitleLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "sectionTitle", nullptr)));
        recordsHintLabel->setText(QCoreApplication::translate("MainWindow", "\345\216\237\345\233\276\343\200\201\347\273\223\346\236\234\345\233\276\345\217\212\345\203\217\347\264\240\345\217\202\346\225\260\347\224\261 SQLite \346\225\260\346\215\256\345\261\202\350\275\275\345\205\245", nullptr));
        recordsHintLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "muted", nullptr)));
        QTableWidgetItem *___qtablewidgetitem = recordsTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("MainWindow", "\350\256\260\345\275\225\347\274\226\345\217\267", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = recordsTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("MainWindow", "\346\213\215\346\221\204\346\227\266\351\227\264", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = recordsTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("MainWindow", "\350\257\206\345\210\253\347\273\223\346\236\234", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = recordsTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("MainWindow", "\347\275\256\344\277\241\345\272\246", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = recordsTable->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("MainWindow", "\346\200\273\345\203\217\347\264\240", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = recordsTable->horizontalHeaderItem(5);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("MainWindow", "\347\274\272\351\231\267\345\203\217\347\264\240", nullptr));
        QTableWidgetItem *___qtablewidgetitem6 = recordsTable->horizontalHeaderItem(6);
        ___qtablewidgetitem6->setText(QCoreApplication::translate("MainWindow", "\347\274\272\351\231\267\345\215\240\346\257\224", nullptr));
        QTableWidgetItem *___qtablewidgetitem7 = recordsTable->horizontalHeaderItem(7);
        ___qtablewidgetitem7->setText(QCoreApplication::translate("MainWindow", "\345\244\207\346\263\250", nullptr));
        deviceStatusDot->setProperty("state", QVariant(QCoreApplication::translate("MainWindow", "online", nullptr)));
        deviceStatusDot->setText(QString());
        deviceStatusLabel->setText(QCoreApplication::translate("MainWindow", "\350\256\276\345\244\207\346\255\243\345\270\270", nullptr));
        storageNameLabel->setText(QCoreApplication::translate("MainWindow", "\345\217\257\347\224\250\345\255\230\345\202\250", nullptr));
        storageNameLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "muted", nullptr)));
        storageValueLabel->setText(QCoreApplication::translate("MainWindow", "42.6 GB", nullptr));
        storageValueLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricValue", nullptr)));
        snapshotCountNameLabel->setText(QCoreApplication::translate("MainWindow", "\346\234\254\346\254\241\346\213\215\347\205\247", nullptr));
        snapshotCountNameLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "muted", nullptr)));
        snapshotCountValueLabel->setText(QCoreApplication::translate("MainWindow", "0", nullptr));
        snapshotCountValueLabel->setProperty("role", QVariant(QCoreApplication::translate("MainWindow", "metricValue", nullptr)));
        recordsNavButton->setText(QCoreApplication::translate("MainWindow", "\346\237\245\347\234\213\346\243\200\346\265\213\350\256\260\345\275\225", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
