#include "mainwindow.h"
#include "patientmonitor.h"
#include "smarttriage.h"
#include "core/triageengine.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QMessageBox>
#include <QIcon>
#include <QPushButton>
#include <QStatusBar>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , trayIcon(nullptr)
    , trayMenu(nullptr)
    , isOfflineMode(false)
    , smartTriage(nullptr)
{
    setWindowTitle("智能桌面医疗机器人");
    resize(1000, 750);

    // ----- Tab 控件（监护仪 + 智能导诊）-----
    QTabWidget *tabWidget = new QTabWidget(this);

    PatientMonitor *patientMonitor = new PatientMonitor(this);
    smartTriage = new SmartTriage(this);   // 存储指针供后续使用

    QFont tabFont = tabWidget->font();
    tabFont.setPointSize(14); // 改成想要的数值
    tabWidget->setFont(tabFont);
    tabWidget->addTab(patientMonitor, "监护仪");
    tabWidget->addTab(smartTriage, "智能导诊");

    // ----- 右上角按钮组（离线模式 + 分诊模式）-----
    QWidget *cornerWidget = new QWidget(this);
    QHBoxLayout *cornerLayout = new QHBoxLayout(cornerWidget);
    cornerLayout->setContentsMargins(0, 0, 0, 0);
    cornerLayout->setSpacing(8);

    // ----- 离线/联网模式按钮-----
    modeBtn = new QPushButton("离线模式", this);
    modeBtn->setCheckable(true);
    modeBtn->setChecked(false);
    modeBtn->setFont(QFont("", 10, QFont::Bold));
    modeBtn->setFixedHeight(30);
    modeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #e8f0fe;
            color: #1a73e8;
            border: 2px solid #1a73e8;
            border-radius: 8px;
            padding: 4px 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #d2e3fc;
        }
        QPushButton:checked {
            background-color: #edfce6;
            border-color: #108f23;
            color: #108f23;
        }
    )");
    connect(modeBtn, &QPushButton::clicked, this, &MainWindow::toggleMode);

    // ----- 分诊模式切换按钮（默认多轮预诊）-----
    triageModeBtn = new QPushButton("多轮预诊", this);
    triageModeBtn->setCheckable(true);
    triageModeBtn->setChecked(true);   // 默认选中多轮预诊
    triageModeBtn->setFont(QFont("", 10, QFont::Bold));
    triageModeBtn->setFixedHeight(32);
    triageModeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #e8f0fe;
            color: #1a73e8;
            border: 2px solid #1a73e8;
            border-radius: 8px;
            padding: 4px 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #d2e3fc;
        }
        QPushButton:checked {
            background-color: #e6f4ea;
            border-color: #34a853;
            color: #1e7e34;
        }
    )");
    connect(triageModeBtn, &QPushButton::clicked, this, &MainWindow::toggleTriageMode);

    // 将两个按钮添加到水平布局
    cornerLayout->addWidget(modeBtn);
    cornerLayout->addWidget(triageModeBtn);
    cornerWidget->setLayout(cornerLayout);

    // 将 cornerWidget 设置为 Tab 栏的右上角控件
    tabWidget->setCornerWidget(cornerWidget, Qt::TopRightCorner);

    setCentralWidget(tabWidget);

    // 初始化模式：默认生产模式（联网）
    TriageEngine::instance().setProductionMode(true);

    // ----- 系统托盘 -----
    createTrayIcon();
}

MainWindow::~MainWindow() {}

void MainWindow::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon::fromTheme("applications-games", QIcon()));
    trayIcon->setToolTip("智能桌面医疗机器人");

    trayMenu = new QMenu(this);
    QAction *showAction = trayMenu->addAction("显示主窗口");
    trayMenu->addSeparator();
    QAction *hideAction = trayMenu->addAction("隐藏到托盘");
    trayMenu->addSeparator();
    QAction *quitAction = trayMenu->addAction("退出");

    connect(showAction, &QAction::triggered, this, &MainWindow::showWindow);
    connect(hideAction, &QAction::triggered, this, &MainWindow::hide);
    connect(quitAction, &QAction::triggered, this, &MainWindow::quitApp);

    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    trayIcon->show();

    trayIcon->showMessage("智能医疗助手", "程序已启动，可最小化到托盘",
                          QSystemTrayIcon::Information, 2000);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon && trayIcon->isVisible()) {
        hide();
        trayIcon->showMessage("智能医疗助手", "程序已最小化到系统托盘",
                              QSystemTrayIcon::Information, 1500);
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        showWindow();
    }
}

void MainWindow::showWindow()
{
    showNormal();
    raise();
    activateWindow();
}

void MainWindow::quitApp()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "确认退出",
        "确定要退出智能医疗助手吗？",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (trayIcon) {
            trayIcon->hide();
        }
        QApplication::quit();
    }
}

// ========== 离线/联网模式切换 ==========
void MainWindow::toggleMode()
{
    bool isOffline = modeBtn->isChecked();
    TriageEngine::instance().setProductionMode(!isOffline);

    if (isOffline) {
        modeBtn->setText("联网模式");
        modeBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #edfce6;
                color: #108f23;
                border: 2px solid #108f23;
                border-radius: 8px;
                padding: 4px 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #ddfacf;
            }
        )");
        statusBar()->showMessage("已切换到离线模式（本地分诊）", 2000);
    } else {
        modeBtn->setText("离线模式");
        modeBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #e8f0fe;
                color: #1a73e8;
                border: 2px solid #1a73e8;
                border-radius: 8px;
                padding: 4px 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #d2e3fc;
            }
        )");
        statusBar()->showMessage("已切换到联网模式（AI分诊）", 2000);
    }
}

// ========== 分诊模式切换（快速分诊 vs 多轮预诊）==========
void MainWindow::toggleTriageMode()
{
    bool isMultiRound = triageModeBtn->isChecked();
    if (smartTriage) {
        smartTriage->setMultiRoundMode(isMultiRound);
    }
    triageModeBtn->setText(isMultiRound ? "多轮预诊" : "快速分诊");
    statusBar()->showMessage(isMultiRound ? "已切换到多轮预诊模式" : "已切换到快速分诊模式", 2000);
}