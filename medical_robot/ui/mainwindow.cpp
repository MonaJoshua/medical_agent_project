#include "mainwindow.h"
#include "thememanager.h"
#include "patientmonitor.h"
#include "smarttriage.h"
#include "voicechatwidget.h"
#include "ui/moretoolswidget.h"
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
    , smartTriage(nullptr)
    , voiceChat(nullptr)
    , patientMonitor(nullptr)
    , darkModeBtn(nullptr)
    , m_isDarkMode(false)
{
    setWindowTitle("智能就医助手");
    resize(1000, 750);

    // ----- Tab 控件 -----
    QTabWidget *tabWidget = new QTabWidget(this);

    // Tab 1: 语音助手（主界面）
    voiceChat = new VoiceChatWidget(this);
    connect(voiceChat, &VoiceChatWidget::requestMonitorPage,
            this, &MainWindow::switchToMonitor);
    // 连接跳转到挂号记录功能的信号
    connect(voiceChat, &VoiceChatWidget::requestSwitchToMoreTools,
            this, [this]() {
                QTabWidget *tabWidget = qobject_cast<QTabWidget*>(centralWidget());
                if (tabWidget) {
                    tabWidget->setCurrentIndex(2); // 切换到更多工具
                    MoreToolsWidget *moreTools = qobject_cast<MoreToolsWidget*>(tabWidget->widget(2));
                    if (moreTools) {
                        moreTools->showRecordsPage();
                    }
                }
            });

    // Tab 2: 一键挂号
    smartTriage = new SmartTriage(this);

    // Tab 3: 更多工具
    MoreToolsWidget *moreTools = new MoreToolsWidget(this);

    // Tab 4: 监护仪
    patientMonitor = new PatientMonitor(this);
    // 连接监护仪历史数据信号，跳转到监护数据页面
    connect(patientMonitor, &PatientMonitor::requestMonitorDataPage, [this, moreTools]() {
        QTabWidget *tabWidget = qobject_cast<QTabWidget*>(centralWidget());
        if (tabWidget) {
            tabWidget->setCurrentIndex(2); // 更多工具索引为2
            if (moreTools) {
                moreTools->showMonitorData();
            }
        }
    });

    QFont tabFont = tabWidget->font();
    tabFont.setPointSize(14);
    tabWidget->setFont(tabFont);
    tabWidget->addTab(voiceChat, "语音助手");
    tabWidget->addTab(smartTriage, "一键挂号");
    tabWidget->addTab(moreTools, "更多工具");
    tabWidget->addTab(patientMonitor, "监护仪");
    tabWidget->setCurrentIndex(0); // 默认语音助手

    // ----- 右上角按钮（深色模式切换） -----
    QWidget *cornerWidget = new QWidget(this);
    QHBoxLayout *cornerLayout = new QHBoxLayout(cornerWidget);
    cornerLayout->setContentsMargins(0, 0, 0, 0);
    cornerLayout->setSpacing(8);

    darkModeBtn = new QPushButton("🌙 深色", this);
    darkModeBtn->setCheckable(true);
    darkModeBtn->setChecked(false);
    darkModeBtn->setFont(QFont("", 10, QFont::Bold));
    darkModeBtn->setFixedHeight(30);
    darkModeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #e8f0fe;
            color: #1a73e8;
            border: 2px solid #1a73e8;
            border-radius: 8px;
            padding: 4px 14px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #d2e3fc; }
        QPushButton:checked {
            background-color: #2d2d2d;
            color: #e0e0e0;
            border-color: #555;
        }
    )");
    connect(darkModeBtn, &QPushButton::toggled, this, &MainWindow::toggleDarkMode);

    cornerLayout->addWidget(darkModeBtn);
    cornerWidget->setLayout(cornerLayout);
    tabWidget->setCornerWidget(cornerWidget, Qt::TopRightCorner);

    setCentralWidget(tabWidget);

    // ----- 系统托盘 -----
    createTrayIcon();
}

MainWindow::~MainWindow() {}

void MainWindow::createTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon::fromTheme("applications-games", QIcon()));
    trayIcon->setToolTip("智能就医助手");

    trayMenu = new QMenu(this);
    QAction *showAction = trayMenu->addAction("显示窗口");
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
    trayIcon->showMessage("医疗助手", "程序已启动，可最小化到托盘运行",
                          QSystemTrayIcon::Information, 2000);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon && trayIcon->isVisible()) {
        hide();
        trayIcon->showMessage("医疗助手", "已最小化到系统托盘",
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
        "确定要退出医疗助手吗？",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (trayIcon) trayIcon->hide();
        QApplication::quit();
    }
}

void MainWindow::toggleDarkMode(bool checked)
{
    m_isDarkMode = checked;
    if (checked) {
        ThemeManager::instance().applyTheme(ThemeManager::Dark);
    } else {
        ThemeManager::instance().applyTheme(ThemeManager::Light);
    }
    darkModeBtn->setText(checked ? "☀️ 浅色" : "🌙 深色");
}

void MainWindow::switchToMonitor()
{
    QTabWidget *tabWidget = qobject_cast<QTabWidget*>(centralWidget());
    if (tabWidget) {
        tabWidget->setCurrentIndex(3); // 监护仪 Tab 索引为 3
    }
}