// monitordatawidget.cpp
// 监护历史数据表格组件

#include "monitordatawidget.h"
#include "thememanager.h"
#include "trendwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QMessageBox>
#include <QDialog>
#include <QCoreApplication>

MonitorDataWidget::MonitorDataWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 头部布局：返回 + 标题 + 趋势图 + 刷新
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(12, 8, 12, 8);
    headerLayout->setSpacing(16);

    QPushButton *backBtn = new QPushButton("返回");
    backBtn->setFixedHeight(50);
    backBtn->setStyleSheet(getButtonStyleSheet());
    connect(backBtn, &QPushButton::clicked, this, &MonitorDataWidget::backToGrid);
    headerLayout->addWidget(backBtn);

    headerLayout->addStretch();

    QLabel *title = new QLabel("监护数据");
    title->setFont(QFont("", 16, QFont::Bold));
    headerLayout->addWidget(title);

    headerLayout->addStretch();

    QPushButton *trendBtn = new QPushButton("📈 趋势图");
    trendBtn->setFixedHeight(50);
    trendBtn->setFont(QFont("", 16, QFont::Medium));
    trendBtn->setStyleSheet(
        "QPushButton { background-color: #1a73e8; color: white; border: none; border-radius: 6px; padding: 0 20px; font-weight: 500; font-size: 16px; }"
        "QPushButton:hover { background-color: #1557b0; }"
    );
    connect(trendBtn, &QPushButton::clicked, this, &MonitorDataWidget::onTrendButtonClicked);
    headerLayout->addWidget(trendBtn);

    QPushButton *refreshBtn = new QPushButton("刷新");
    refreshBtn->setFixedHeight(50);
    refreshBtn->setStyleSheet(getButtonStyleSheet());
    connect(refreshBtn, &QPushButton::clicked, this, &MonitorDataWidget::refreshData);
    headerLayout->addWidget(refreshBtn);

    mainLayout->addLayout(headerLayout);

    // 数据表格（5列）
    recordTable = new QTableWidget(this);
    recordTable->setColumnCount(5);
    recordTable->setHorizontalHeaderLabels({"日期", "时间段", "心率 (bpm)", "血氧 (%)", "体温 (°C)"});
    recordTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    recordTable->horizontalHeader()->setFont(QFont("", 18, QFont::Bold));
    recordTable->verticalHeader()->setVisible(false);
    recordTable->setSelectionMode(QAbstractItemView::NoSelection);
    recordTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    recordTable->setShowGrid(false);
    recordTable->setFont(QFont("", 18));
    recordTable->setMinimumHeight(400);
    mainLayout->addWidget(recordTable);

    loadData();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &MonitorDataWidget::updateTableStyle);
}

QString MonitorDataWidget::getButtonStyleSheet() const
{
    bool isDark = (ThemeManager::instance().currentTheme() == ThemeManager::Dark);
    QString borderColor = isDark ? "#6a8aff" : "#1a73e8";
    QString textColor = isDark ? "#6a8aff" : "#1a73e8";
    QString hoverBg = isDark ? "#6a8aff" : "#1a73e8";
    QString hoverText = isDark ? "#1a1a1a" : "#ffffff";

    return QString(
        "QPushButton { background-color: transparent; border: 1px solid %1; border-radius: 6px; padding: 8px 24px; font-size: 16px; font-weight: 500; min-width: 70px; color: %2; }"
        "QPushButton:hover { background-color: %3; color: %4; }"
    ).arg(borderColor, textColor, hoverBg, hoverText);
}

void MonitorDataWidget::loadData()
{
    recordTable->setUpdatesEnabled(false);

    // 确保数据库连接和表存在
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        db = QSqlDatabase::addDatabase("QSQLITE");
        QString dbPath = QCoreApplication::applicationDirPath() + "/patient_monitor.db";
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            showError("无法打开数据库，请检查权限");
            return;
        }

        QSqlQuery query;
        query.exec("CREATE TABLE IF NOT EXISTS vital_data ("
                   "timestamp INTEGER PRIMARY KEY,"
                   "heart_rate REAL,"
                   "spo2 REAL,"
                   "temperature REAL,"
                   "hr_max REAL, hr_min REAL, hr_std REAL,"
                   "spo2_max REAL, spo2_min REAL, spo2_std REAL,"
                   "temp_max REAL, temp_min REAL, temp_std REAL,"
                   "count INTEGER"
                   ")");
        if (query.lastError().isValid()) {
            showError("创建数据表失败：" + query.lastError().text());
            return;
        }
    }

    recordTable->clearContents();
    recordTable->setRowCount(0);
    recordTable->setColumnCount(5);
    recordTable->setHorizontalHeaderLabels({"日期", "时间段", "心率 (bpm)", "血氧 (%)", "体温 (°C)"});

    // 查询最近 100 条记录
    QSqlQuery query;
    query.prepare("SELECT timestamp, heart_rate, spo2, temperature FROM vital_data ORDER BY timestamp DESC LIMIT 100");
    if (!query.exec()) {
        showError("查询失败：" + query.lastError().text());
        return;
    }

    QList<QPair<qint64, QPair<double, QPair<double, double>>>> dataList;
    while (query.next()) {
        qint64 ts = query.value(0).toLongLong();
        if (ts < 10000000000) ts *= 1000;
        dataList.append(qMakePair(ts, qMakePair(
            query.value(1).toDouble(),
            qMakePair(query.value(2).toDouble(), query.value(3).toDouble())
        )));
    }

    if (dataList.isEmpty()) {
        showMessage("暂无监护数据，请先进行测量");
        return;
    }

    // 填充表格
    recordTable->setRowCount(dataList.size());
    for (int row = 0; row < dataList.size(); ++row) {
        const auto &item = dataList[row];
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(item.first);
        QString dateStr = dt.toString("yyyy-MM-dd");
        QString timeStr = dt.toString("hh:mm");
        double hr = item.second.first;
        double spo2 = item.second.second.first;
        double temp = item.second.second.second;

        recordTable->setItem(row, 0, makeTableItem(dateStr));
        recordTable->setItem(row, 1, makeTableItem(timeStr + " - " + timeStr));
        recordTable->setItem(row, 2, makeTableItem(QString::number(hr, 'f', 0)));
        recordTable->setItem(row, 3, makeTableItem(QString::number(spo2, 'f', 0)));
        recordTable->setItem(row, 4, makeTableItem(QString::number(temp, 'f', 1)));
    }

    for (int row = 0; row < recordTable->rowCount(); ++row) {
        recordTable->setRowHeight(row, 55);
    }

    recordTable->setColumnWidth(0, 200);
    recordTable->setColumnWidth(1, 240);
    recordTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    recordTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    recordTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    recordTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    recordTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);

    recordTable->setUpdatesEnabled(true);
    updateTableStyle();
}

void MonitorDataWidget::showError(const QString &msg)
{
    recordTable->setRowCount(1);
    QTableWidgetItem *item = new QTableWidgetItem(msg);
    item->setTextAlignment(Qt::AlignCenter);
    recordTable->setItem(0, 0, item);
    recordTable->setSpan(0, 0, 1, 5);
    recordTable->setUpdatesEnabled(true);
}

void MonitorDataWidget::showMessage(const QString &msg)
{
    recordTable->setRowCount(1);
    QTableWidgetItem *item = new QTableWidgetItem(msg);
    item->setTextAlignment(Qt::AlignCenter);
    recordTable->setItem(0, 0, item);
    recordTable->setSpan(0, 0, 1, 5);
    recordTable->setUpdatesEnabled(true);
}

QTableWidgetItem* MonitorDataWidget::makeTableItem(const QString &text)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return item;
}

void MonitorDataWidget::updateTableStyle()
{
    bool isDark = (ThemeManager::instance().currentTheme() == ThemeManager::Dark);

    QString style = isDark ?
        "QTableWidget { background-color: #1e1e1e; color: #d0d8e8; gridline-color: #3a4a5a; }"
        "QTableWidget::item { background-color: transparent; padding: 8px; }"
        "QHeaderView::section { background-color: #2a2a3a; color: #b0b8c8; padding: 10px; border: none; border-bottom: 2px solid #4a5a7a; font-size: 18px; font-weight: bold; }" :
        "QTableWidget { background-color: #ffffff; color: #2c3e50; gridline-color: #e8edf2; }"
        "QTableWidget::item { background-color: transparent; padding: 8px; }"
        "QHeaderView::section { background-color: #e8edf2; color: #2c3e50; padding: 10px; border: none; border-bottom: 2px solid #d0d8e0; font-size: 18px; font-weight: bold; }";
    recordTable->setStyleSheet(style);

    for (QPushButton *btn : findChildren<QPushButton*>()) {
        if (btn->text() != "趋势图") {
            btn->setStyleSheet(getButtonStyleSheet());
        }
    }
}

void MonitorDataWidget::refreshData()
{
    loadData();
}

void MonitorDataWidget::onTrendButtonClicked()
{
    if (!m_trendWindow.isNull() && m_trendWindow->isVisible()) {
        m_trendWindow->raise();
        m_trendWindow->activateWindow();
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("选择趋势图参数");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *label = new QLabel("选择要查看的趋势图：");
    label->setAlignment(Qt::AlignCenter);
    label->setFont(QFont("", 16));
    layout->addWidget(label);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QString btnStyle = "QPushButton { background-color: #1a73e8; color: white; border: none; border-radius: 6px; padding: 10px 30px; font-size: 16px; font-weight: bold; min-height: 44px; }"
                       "QPushButton:hover { background-color: #1557b0; }";

    QPushButton *hrBtn = new QPushButton("心率");
    QPushButton *spo2Btn = new QPushButton("血氧");
    QPushButton *tempBtn = new QPushButton("体温");
    hrBtn->setStyleSheet(btnStyle);
    spo2Btn->setStyleSheet(btnStyle);
    tempBtn->setStyleSheet(btnStyle);

    connect(hrBtn, &QPushButton::clicked, [&dialog, this]() {
        dialog.accept();
        showTrendWindow("heartRate", "心率");
    });
    connect(spo2Btn, &QPushButton::clicked, [&dialog, this]() {
        dialog.accept();
        showTrendWindow("spo2", "血氧");
    });
    connect(tempBtn, &QPushButton::clicked, [&dialog, this]() {
        dialog.accept();
        showTrendWindow("temperature", "体温");
    });

    btnLayout->addWidget(hrBtn);
    btnLayout->addWidget(spo2Btn);
    btnLayout->addWidget(tempBtn);
    layout->addLayout(btnLayout);

    dialog.exec();
}

void MonitorDataWidget::showTrendWindow(const QString &paramName, const QString &displayName)
{
    if (!m_trendWindow.isNull() && m_trendWindow->isVisible()) {
        m_trendWindow->raise();
        m_trendWindow->activateWindow();
        return;
    }

    TrendWidget *trend = new TrendWidget(paramName);
    trend->setWindowTitle(QString("趋势图 - %1").arg(displayName));
    trend->resize(800, 500);
    trend->setAttribute(Qt::WA_DeleteOnClose);
    trend->setWindowFlags(Qt::Window);
    trend->show();

    m_trendWindow = trend;
    connect(trend, &QObject::destroyed, [this]() {
        m_trendWindow.clear();
    });
}