#include "patientmonitor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QDebug>
#include <QRandomGenerator>

// ========== PatientMonitor 实现 ==========
PatientMonitor::PatientMonitor(QWidget *parent)
    : QWidget(parent)
{
    // 创建布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 波形控件
    waveWidget = new WaveWidget(this);
    mainLayout->addWidget(waveWidget);
    
    // 数值显示区域
    QHBoxLayout *valueLayout = new QHBoxLayout();
    hrLCD = new QLCDNumber();
    hrLCD->setDigitCount(3);
    hrLCD->setSegmentStyle(QLCDNumber::Filled);
    
    spo2LCD = new QLCDNumber();
    spo2LCD->setDigitCount(3);
    
    tempLCD = new QLCDNumber();
    tempLCD->setDigitCount(4);
    tempLCD->setSmallDecimalPoint(true);
    
    valueLayout->addWidget(new QLabel("❤️ 心率:"));
    valueLayout->addWidget(hrLCD);
    valueLayout->addWidget(new QLabel("💨 血氧:"));
    valueLayout->addWidget(spo2LCD);
    valueLayout->addWidget(new QLabel("🌡️ 体温:"));
    valueLayout->addWidget(tempLCD);
    mainLayout->addLayout(valueLayout);
    
    // 报警灯
    alarmLight = new QLabel();
    alarmLight->setFixedSize(40, 40);
    alarmLight->setStyleSheet("background-color: gray; border-radius: 20px;");
    mainLayout->addWidget(alarmLight, 0, Qt::AlignCenter);
    
    // 紧急按钮
    QPushButton *emergencyBtn = new QPushButton("🚨 紧急呼叫");
    emergencyBtn->setFont(QFont("", 14));
    mainLayout->addWidget(emergencyBtn);
    
    connect(emergencyBtn, &QPushButton::clicked, []() {
        qDebug() << "🔔 Emergency call triggered!";
    });
    
    // 定时器
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &PatientMonitor::updateData);
    timer->start(50);
    
    // 初始显示
    hrLCD->display(75);
    spo2LCD->display(98);
    tempLCD->display(36.5);
}

PatientMonitor::~PatientMonitor()
{
    if (timer) {
        timer->stop();
        delete timer;
        timer = nullptr;
    }
}

void PatientMonitor::updateData()
{
    if (!timer || !waveWidget || !hrLCD || !spo2LCD || !tempLCD || !alarmLight) {
        return;
    }
    
    static int heartRate = 75;
    heartRate += QRandomGenerator::global()->bounded(-3, 4);
    heartRate = qBound(60, heartRate, 110);
    
    int spo2 = QRandomGenerator::global()->bounded(95, 101);
    double temp = 36.5 + QRandomGenerator::global()->bounded(-10, 15) / 10.0;
    temp = qBound(36.0, temp, 38.5);
    
    hrLCD->display(heartRate);
    spo2LCD->display(spo2);
    tempLCD->display(temp);
    
    waveWidget->addDataPoint(heartRate);
    
    bool crisis = (heartRate > 100 || spo2 < 93 || temp > 37.5);
    if (crisis) {
        alarmLight->setStyleSheet("background-color: red; border-radius: 20px;");
        qDebug() << "⚠️ ALARM! HR:" << heartRate << "SpO2:" << spo2 << "Temp:" << temp;
    } else {
        alarmLight->setStyleSheet("background-color: green; border-radius: 20px;");
    }
}