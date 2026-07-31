#include "patientmonitor.h"
#include "thememanager.h"
#include "core/datamanager.h"
#include "trendwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QDebug>
#include <QRandomGenerator>
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QTimer>
#include <QMessageBox>
#include <QCoreApplication>

PatientMonitor::PatientMonitor(QWidget *parent)
    : QWidget(parent)
    , waveWidget(nullptr)
    , hrLCD(nullptr)
    , spo2LCD(nullptr)
    , tempLCD(nullptr)
    , m_hrTimeLabel(nullptr)
    , m_spo2TimeLabel(nullptr)
    , m_tempTimeLabel(nullptr)
    , alarmLight(nullptr)
    , alarmSettingsBtn(nullptr)
    , startBtn(nullptr)
    , m_collectTimer(nullptr)
    , m_isCollecting(false)
    , m_collectInterval(2000)
    , m_isDark(false)
    , m_blinkTimer(nullptr)
    , m_storeTimer(new QTimer(this))
    , m_maxBufferSize(20)
{
    setupUI();
    updateTheme();

    // 连接主题切换
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &PatientMonitor::updateTheme);

    // 连接数据信号
    connect(&DataManager::instance(), &DataManager::newDataAvailable,
            this, &PatientMonitor::onDataReceived);

    // 初始化数据库
    initDatabase();

    // 定时存储
    connect(m_storeTimer, &QTimer::timeout, this, &PatientMonitor::flushBuffer);
    m_storeTimer->start(10000);

    // 加载最近一次测量值
    updateLastMeasurement();
}

PatientMonitor::~PatientMonitor()
{
    if (m_collectTimer) {
        m_collectTimer->stop();
        delete m_collectTimer;
        m_collectTimer = nullptr;
    }

    if (m_blinkTimer) {
        m_blinkTimer->stop();
        delete m_blinkTimer;
        m_blinkTimer = nullptr;
    }

    if (m_storeTimer) {
        m_storeTimer->stop();
        delete m_storeTimer;
        m_storeTimer = nullptr;
    }
}

void PatientMonitor::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 20, 24, 20);

    // ---- 波形 ----
    waveWidget = new WaveWidget(this);
    waveWidget->setMinimumHeight(150);
    waveWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(waveWidget);

    // ---- 数值卡片（3个指标） ----
    QGridLayout *cardLayout = new QGridLayout();
    cardLayout->setSpacing(20);
    cardLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *hrCard = createValueCard("❤️ 心率", hrLCD, m_hrTimeLabel);
    cardLayout->addWidget(hrCard, 0, 0);
    QWidget *spo2Card = createValueCard("💨 血氧", spo2LCD, m_spo2TimeLabel);
    cardLayout->addWidget(spo2Card, 0, 1);
    QWidget *tempCard = createValueCard("🌡️ 体温", tempLCD, m_tempTimeLabel);
    cardLayout->addWidget(tempCard, 0, 2);
    mainLayout->addLayout(cardLayout);

    // ---- 底部操作栏 ----
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(20);

    // 报警灯
    alarmLight = new QLabel();
    alarmLight->setFixedSize(50, 50);
    alarmLight->setStyleSheet("background-color: #4caf50; border-radius: 25px;");
    bottomLayout->addWidget(alarmLight, 0, Qt::AlignLeft);
    bottomLayout->addStretch();

    // 开始测量
    startBtn = new QPushButton("开始测量");
    startBtn->setFont(QFont("", 18, QFont::Bold));
    startBtn->setMinimumHeight(48);
    startBtn->setCursor(Qt::PointingHandCursor);
    connect(startBtn, &QPushButton::clicked, this, &PatientMonitor::onStartMeasurementClicked);
    bottomLayout->addWidget(startBtn, 0, Qt::AlignRight);

    // 采集定时器
    m_collectTimer = new QTimer(this);
    m_collectTimer->setInterval(m_collectInterval);
    connect(m_collectTimer, &QTimer::timeout, this, &PatientMonitor::onCollectTimerTimeout);

    // 历史数据
    QPushButton *historyBtn = new QPushButton("📝 历史数据");
    historyBtn->setObjectName("historyBtn");
    historyBtn->setFont(QFont("", 18));
    historyBtn->setMinimumHeight(48);
    historyBtn->setCursor(Qt::PointingHandCursor);
    connect(historyBtn, &QPushButton::clicked, this, &PatientMonitor::onHistoryClicked);
    bottomLayout->addWidget(historyBtn);

    // 阈值设置
    alarmSettingsBtn = new QPushButton("⚙️ 阈值设置");
    alarmSettingsBtn->setFont(QFont("", 18));
    alarmSettingsBtn->setMinimumHeight(48);
    alarmSettingsBtn->setCursor(Qt::PointingHandCursor);
    connect(alarmSettingsBtn, &QPushButton::clicked, this, &PatientMonitor::showAlarmSettings);
    bottomLayout->addWidget(alarmSettingsBtn);

    mainLayout->addLayout(bottomLayout);
}

QWidget* PatientMonitor::createValueCard(const QString &title, QLCDNumber *&lcd, QLabel *&timeLabel)
{
    QWidget *card = new QWidget();
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setSpacing(6);
    layout->setContentsMargins(12, 12, 12, 12);

    // 标题行（标题 + 时间）
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel(title);
    titleLabel->setFont(QFont("", 16, QFont::Bold));
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    timeLabel = new QLabel();
    timeLabel->setFont(QFont("", 13));
    timeLabel->setStyleSheet("color: #8899aa;");
    titleLayout->addWidget(timeLabel);

    layout->addLayout(titleLayout);

    // LCD数字
    lcd = new QLCDNumber();
    lcd->setDigitCount(3);
    lcd->setSegmentStyle(QLCDNumber::Filled);
    lcd->setMinimumHeight(60);
    lcd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(lcd);

    // 单位标签
    QLabel *unitLabel = nullptr;
    if (title.contains("心率")) {
        unitLabel = new QLabel("bpm");
    } else if (title.contains("血氧")) {
        unitLabel = new QLabel("%");
    } else {
        unitLabel = new QLabel("°C");
        lcd->setDigitCount(4);
        lcd->setSmallDecimalPoint(true);
    }
    if (unitLabel) {
        unitLabel->setFont(QFont("", 14));
        unitLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(unitLabel);
    }

    card->setStyleSheet(
        "QWidget { background-color: transparent; border: 1px solid #d5e4ed; border-radius: 12px; }"
    );
    return card;
}

void PatientMonitor::updateTheme()
{
    bool isDark = (ThemeManager::instance().currentTheme() == ThemeManager::Dark);
    m_isDark = isDark;

    // ---- 卡片背景 ----
    QList<QWidget*> cards = findChildren<QWidget*>();
    for (QWidget *w : cards) {
        if (w->inherits("QWidget") && !w->inherits("QLCDNumber") && !w->inherits("QLabel") && !w->inherits("QPushButton")) {
            if (w->styleSheet().contains("border-radius")) {
                QString borderColor = isDark ? "#4a5a7a" : "#d5e4ed";
                QString bgColor = isDark ? "#2a2a3a" : "#ffffff";
                w->setStyleSheet(QString("QWidget { background-color: %1; border: 1px solid %2; border-radius: 12px; }")
                                 .arg(bgColor, borderColor));
            }
        }
    }

    // ---- 阈值设置按钮 ----
    if (alarmSettingsBtn) {
        QString style = isDark ?
            "QPushButton { background-color: #2a2a3a; color: #d0d8e8; border: 1px solid #4a5a7a; border-radius: 6px; padding: 8px 18px; font-size: 18px; font-weight: bold; min-height: 48px; }"
            "QPushButton:hover { background-color: #3a3a5a; }" :
            "QPushButton { background-color: #f0f4f8; color: #1a73e8; border: 1px solid #d5e4ed; border-radius: 6px; padding: 8px 18px; font-size: 18px; font-weight: bold; min-height: 48px; }"
            "QPushButton:hover { background-color: #e0e8f0; }";
        alarmSettingsBtn->setStyleSheet(style);
    }

    // ---- 历史数据按钮 ----
    QPushButton *historyBtn = findChild<QPushButton*>("historyBtn");
    if (historyBtn) {
        QString style = isDark ?
            "QPushButton { background-color: #2a2a3a; color: #d0d8e8; border: 1px solid #4a5a7a; border-radius: 6px; padding: 8px 18px; font-size: 18px; font-weight: bold; min-height: 48px; }"
            "QPushButton:hover { background-color: #3a3a5a; }" :
            "QPushButton { background-color: #f0f4f8; color: #1a73e8; border: 1px solid #d5e4ed; border-radius: 6px; padding: 8px 18px; font-size: 18px; font-weight: bold; min-height: 48px; }"
            "QPushButton:hover { background-color: #e0e8f0; }";
        historyBtn->setStyleSheet(style);
    }

    // ---- 开始/停止 ----
    if (startBtn) {
        QString style;
        if (m_isCollecting) {
            style = isDark ?
                "QPushButton { background-color: #b71c1c; color: white; border: none; border-radius: 8px; padding: 8px 24px; font-size: 18px; font-weight: bold; min-height: 48px; }"
                "QPushButton:hover { background-color: #d32f2f; }" :
                "QPushButton { background-color: #d93025; color: white; border: none; border-radius: 8px; padding: 8px 24px; font-size: 18px; font-weight: bold; min-height: 48px; }"
                "QPushButton:hover { background-color: #b71c1c; }";
        } else {
            style = isDark ?
                "QPushButton { background-color: #1a3a6a; color: #d0d8e8; border: none; border-radius: 8px; padding: 8px 24px; font-size: 18px; font-weight: bold; min-height: 48px; }"
                "QPushButton:hover { background-color: #1a4a8a; }" :
                "QPushButton { background-color: #1a73e8; color: white; border: none; border-radius: 8px; padding: 8px 24px; font-size: 18px; font-weight: bold; min-height: 48px; }"
                "QPushButton:hover { background-color: #1557b0; }";
        }
        startBtn->setStyleSheet(style);
    }

    // ---- 波形 ----
    if (waveWidget) waveWidget->update();
}

void PatientMonitor::onDataReceived(const VitalSigns &data)
{
    hrLCD->display((int)data.heartRate);
    spo2LCD->display((int)data.spo2);
    tempLCD->display(data.temperature);
    waveWidget->addDataPoint(data.heartRate);

    // 报警检测
    bool crisis = false;
    if (data.heartRate < m_thresholds.hrMin || data.heartRate > m_thresholds.hrMax ||
        data.spo2 < m_thresholds.spo2Min || data.spo2 > m_thresholds.spo2Max ||
        data.temperature < m_thresholds.tempMin || data.temperature > m_thresholds.tempMax) {
        crisis = true;
    }

    if (crisis) {
        if (!m_blinkTimer) {
            m_blinkTimer = new QTimer(this);
            connect(m_blinkTimer, &QTimer::timeout, [this]() {
                static bool bright = true;
                bright = !bright;
                alarmLight->setStyleSheet(bright ?
                    "background-color: #f44336; border-radius: 25px;" :
                    "background-color: #b71c1c; border-radius: 25px;");
            });
        }
        if (!m_blinkTimer->isActive()) {
            m_blinkTimer->start(500);
            alarmLight->setStyleSheet("background-color: #f44336; border-radius: 25px;");
        }
    } else {
        if (m_blinkTimer && m_blinkTimer->isActive()) {
            m_blinkTimer->stop();
        }
        alarmLight->setStyleSheet("background-color: #4caf50; border-radius: 25px;");
    }

    if (m_isCollecting) {
        m_currentSessionData.append(data);
    }
}

void PatientMonitor::initDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = QCoreApplication::applicationDirPath() + "/patient_monitor.db";
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qWarning() << "无法打开数据库：" << db.lastError().text();
        return;
    }
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS vital_data ("
               "timestamp INTEGER PRIMARY KEY,"
               "heart_rate REAL,"
               "spo2 REAL,"
               "temperature REAL,"
               "hr_max REAL,"
               "hr_min REAL,"
               "hr_std REAL,"
               "spo2_max REAL,"
               "spo2_min REAL,"
               "spo2_std REAL,"
               "temp_max REAL,"
               "temp_min REAL,"
               "temp_std REAL,"
               "count INTEGER"
               ")");
}

void PatientMonitor::flushBuffer()
{
    if (m_dataBuffer.isEmpty()) return;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开，无法存储";
        m_dataBuffer.clear();
        return;
    }

    QSqlQuery query;
    db.transaction();
    for (const VitalSigns &data : m_dataBuffer) {
        query.prepare("INSERT INTO vital_data (timestamp, heart_rate, spo2, temperature) VALUES (?, ?, ?, ?)");
        query.addBindValue(data.timestamp);
        query.addBindValue(data.heartRate);
        query.addBindValue(data.spo2);
        query.addBindValue(data.temperature);
        query.exec();
    }
    db.commit();
    m_dataBuffer.clear();

    qint64 cutoff = QDateTime::currentMSecsSinceEpoch() - 7 * 24 * 3600 * 1000;
    query.prepare("DELETE FROM vital_data WHERE timestamp < ?");
    query.addBindValue(cutoff);
    query.exec();
}

void PatientMonitor::showAlarmSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle("报警阈值设置");
    QFormLayout *form = new QFormLayout(&dialog);
    QSpinBox *hrMin = new QSpinBox; hrMin->setRange(30, 150); hrMin->setValue(m_thresholds.hrMin);
    QSpinBox *hrMax = new QSpinBox; hrMax->setRange(30, 200); hrMax->setValue(m_thresholds.hrMax);
    QSpinBox *spo2Min = new QSpinBox; spo2Min->setRange(70, 100); spo2Min->setValue(m_thresholds.spo2Min);
    QSpinBox *spo2Max = new QSpinBox; spo2Max->setRange(90, 100); spo2Max->setValue(m_thresholds.spo2Max);
    QDoubleSpinBox *tempMin = new QDoubleSpinBox; tempMin->setRange(34, 37); tempMin->setValue(m_thresholds.tempMin);
    QDoubleSpinBox *tempMax = new QDoubleSpinBox; tempMax->setRange(36, 40); tempMax->setValue(m_thresholds.tempMax);

    form->addRow("心率下限 (bpm)", hrMin);
    form->addRow("心率上限 (bpm)", hrMax);
    form->addRow("血氧下限 (%)", spo2Min);
    form->addRow("血氧上限 (%)", spo2Max);
    form->addRow("体温下限 (°C)", tempMin);
    form->addRow("体温上限 (°C)", tempMax);

    QPushButton *okBtn = new QPushButton("确认");
    connect(okBtn, &QPushButton::clicked, [&](){
        m_thresholds.hrMin = hrMin->value();
        m_thresholds.hrMax = hrMax->value();
        m_thresholds.spo2Min = spo2Min->value();
        m_thresholds.spo2Max = spo2Max->value();
        m_thresholds.tempMin = tempMin->value();
        m_thresholds.tempMax = tempMax->value();
        dialog.accept();
    });
    form->addRow(okBtn);
    dialog.exec();
}

void PatientMonitor::showParameterTrend(const QString &paramName)
{
    // 如果已有趋势图窗口且仍可见，则激活它
    if (!m_trendWindow.isNull() && m_trendWindow->isVisible()) {
        m_trendWindow->raise();
        m_trendWindow->activateWindow();
        return;
    }

    // 创建新窗口
    TrendWidget *trend = new TrendWidget(paramName);
    trend->setWindowTitle(QString("趋势图 - %1").arg(paramName));
    trend->resize(800, 500);
    trend->setAttribute(Qt::WA_DeleteOnClose);
    trend->setWindowFlags(Qt::Window);
    trend->show();

    // 保存指针，并在窗口关闭时自动清空
    m_trendWindow = trend;
    connect(trend, &QObject::destroyed, [this]() {
        m_trendWindow.clear();
    });
}

void PatientMonitor::onStartMeasurementClicked()
{
    if (m_isCollecting) {
        m_collectTimer->stop();
        m_isCollecting = false;
        startBtn->setText("开始测量");
        finishMeasurement();
    } else {
        m_currentSessionData.clear();
        m_collectTimer->start();
        m_isCollecting = true;
        startBtn->setText("停止测量");
    }
    updateTheme();
}

void PatientMonitor::onCollectTimerTimeout()
{
    DataManager::instance().recordOnce();
}

void PatientMonitor::finishMeasurement()
{
    int count = m_currentSessionData.size();
    if (count == 0) return;

    double sumHR = 0, sumSpO2 = 0, sumTemp = 0;
    double minHR = 999, maxHR = -999;
    double minSpO2 = 999, maxSpO2 = -999;
    double minTemp = 999, maxTemp = -999;
    QList<double> hrList, spo2List, tempList;

    for (const auto &d : m_currentSessionData) {
        double hr = d.heartRate;
        double spo2 = d.spo2;
        double temp = d.temperature;
        sumHR += hr; sumSpO2 += spo2; sumTemp += temp;
        if (hr < minHR) minHR = hr; if (hr > maxHR) maxHR = hr;
        if (spo2 < minSpO2) minSpO2 = spo2; if (spo2 > maxSpO2) maxSpO2 = spo2;
        if (temp < minTemp) minTemp = temp; if (temp > maxTemp) maxTemp = temp;
        hrList.append(hr); spo2List.append(spo2); tempList.append(temp);
    }

    double avgHR = sumHR / count;
    double avgSpO2 = sumSpO2 / count;
    double avgTemp = sumTemp / count;

    // 计算标准差（可选）
    double hrStd = 0, spo2Std = 0, tempStd = 0;
    for (int i=0; i<count; ++i) {
        hrStd += pow(hrList[i] - avgHR, 2);
        spo2Std += pow(spo2List[i] - avgSpO2, 2);
        tempStd += pow(tempList[i] - avgTemp, 2);
    }
    hrStd = sqrt(hrStd / count);
    spo2Std = sqrt(spo2Std / count);
    tempStd = sqrt(tempStd / count);

    // 保存到数据库
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isOpen()) {
        QSqlQuery query;
        query.prepare("INSERT INTO vital_data (timestamp, heart_rate, spo2, temperature, "
                      "hr_max, hr_min, hr_std, spo2_max, spo2_min, spo2_std, "
                      "temp_max, temp_min, temp_std, count) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(QDateTime::currentMSecsSinceEpoch());
        query.addBindValue(avgHR);
        query.addBindValue(avgSpO2);
        query.addBindValue(avgTemp);
        query.addBindValue(maxHR);
        query.addBindValue(minHR);
        query.addBindValue(hrStd);
        query.addBindValue(maxSpO2);
        query.addBindValue(minSpO2);
        query.addBindValue(spo2Std);
        query.addBindValue(maxTemp);
        query.addBindValue(minTemp);
        query.addBindValue(tempStd);
        query.addBindValue(count);
        if (!query.exec()) {
            qWarning() << "插入测量数据失败：" << query.lastError().text();
        }
    }

    // 更新界面显示最新测量值
    updateLastMeasurement();
    QMessageBox::information(this, "测量完成", QString("本次测量有效点数：%1\n平均值已保存").arg(count));
}

void PatientMonitor::onHistoryClicked()
{
    // 直接跳转到更多工具中的监护数据页面
    emit requestMonitorDataPage();
}

void PatientMonitor::updateLastMeasurement()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qWarning() << "数据库未打开，无法读取最新测量值";
        return;
    }

    QSqlQuery query;
    query.prepare("SELECT timestamp, heart_rate, spo2, temperature FROM vital_data ORDER BY timestamp DESC LIMIT 1");
    if (!query.exec() || !query.next()) {
        hrLCD->display(0);
        spo2LCD->display(0);
        tempLCD->display(0.0);
        if (m_hrTimeLabel) m_hrTimeLabel->setText("");
        if (m_spo2TimeLabel) m_spo2TimeLabel->setText("");
        if (m_tempTimeLabel) m_tempTimeLabel->setText("");
        return;
    }

    qint64 ts = query.value(0).toLongLong();
    if (ts < 10000000000) ts *= 1000;
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(ts);
    QString timeStr = dt.toString("MM/dd hh:mm");

    double hr = query.value(1).toDouble();
    double spo2 = query.value(2).toDouble();
    double temp = query.value(3).toDouble();

    hrLCD->display((int)hr);
    spo2LCD->display((int)spo2);
    tempLCD->display(temp);

    if (m_hrTimeLabel) m_hrTimeLabel->setText(timeStr);
    if (m_spo2TimeLabel) m_spo2TimeLabel->setText(timeStr);
    if (m_tempTimeLabel) m_tempTimeLabel->setText(timeStr);
}