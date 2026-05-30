#include "mainwindow.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("🏥 智能桌面医疗机器人");
    resize(1000, 750);

    // 创建 Tab 控件
    tabWidget = new QTabWidget(this);
    
    // 创建两个页面
    patientMonitor = new PatientMonitor(this);
    smartTriage = new SmartTriage(this);
    
    // 添加 Tab
    tabWidget->addTab(patientMonitor, "📊 病人监护仪");
    tabWidget->addTab(smartTriage, "🤖 智能导诊");
    
    setCentralWidget(tabWidget);
}

MainWindow::~MainWindow() {}
