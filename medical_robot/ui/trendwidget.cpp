#include "trendwidget.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QMessageBox>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QChart>
#include <algorithm>

TrendWidget::TrendWidget(const QString &paramName, QWidget *parent)
    : QWidget(parent), m_paramName(paramName)
{
    setupUI();
    loadData();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, &TrendWidget::applyTheme);
}

TrendWidget::~TrendWidget() {}

void TrendWidget::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 不再添加额外的标题，图表自身会显示标题
    chartView = new QChartView(this);
    chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(chartView);

    applyTheme();
}

void TrendWidget::applyTheme()
{
    bool isDark = (ThemeManager::instance().currentTheme() == ThemeManager::Dark);
    QChart *chart = chartView->chart();
    if (chart) {
        // 背景色（不变）
        chart->setBackgroundBrush(isDark ? QBrush(QColor(30, 30, 30)) : QBrush(QColor(255, 255, 255)));
        
        // 标题颜色改为蓝色
        chart->setTitleBrush(QBrush(QColor(66, 133, 244)));   // Google Blue
        
        // 图例颜色改为蓝色
        chart->legend()->setLabelColor(QColor(66, 133, 244));
        
        // 坐标轴标签颜色改为蓝色
        QList<QAbstractAxis*> axes = chart->axes();
        for (QAbstractAxis *axis : axes) {
            axis->setLabelsColor(QColor(66, 133, 244));
            axis->setTitleBrush(QBrush(QColor(66, 133, 244)));
        }
    }
}

void TrendWidget::loadData()
{
    QChart *chart = new QChart();
    // 只保留一个标题：简洁的“心率今日趋势”
    QString titleName;
    if (m_paramName == "heartRate") titleName = "心率今日趋势";
    else if (m_paramName == "spo2") titleName = "血氧今日趋势";
    else if (m_paramName == "temperature") titleName = "体温今日趋势";
    else titleName = m_paramName + "今日趋势";
    chart->setTitle(titleName);
    chart->setTheme(QChart::ChartThemeLight);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setAnimationOptions(QChart::NoAnimation);

    // ---- 计算当天 00:00 到当前时间 ----
    QDateTime now = QDateTime::currentDateTime();
    QDateTime start(now.date(), QTime(0, 0, 0));
    qint64 startMs = start.toMSecsSinceEpoch();

    // ---- 查询数据库 ----
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        QMessageBox::warning(this, "错误", "数据库未打开！");
        chartView->setChart(chart);
        return;
    }

    QString field;
    QString yTitle;
    if (m_paramName == "heartRate") {
        field = "heart_rate";
        yTitle = "心率 (bpm)";
    } else if (m_paramName == "spo2") {
        field = "spo2";
        yTitle = "血氧 (%)";
    } else if (m_paramName == "temperature") {
        field = "temperature";
        yTitle = "体温 (°C)";
    } else {
        QMessageBox::warning(this, "错误", "未知参数类型");
        chartView->setChart(chart);
        return;
    }

    QSqlQuery query;
    query.prepare(QString("SELECT timestamp, %1 FROM vital_data WHERE timestamp > ? ORDER BY timestamp ASC").arg(field));
    query.addBindValue(startMs);
    if (!query.exec()) {
        QMessageBox::warning(this, "错误", "查询失败：" + query.lastError().text());
        chartView->setChart(chart);
        return;
    }

    // ---- 读取数据 ----
    QList<QPair<qint64, double>> dataList;
    while (query.next()) {
        qint64 ts = query.value(0).toLongLong();
        if (ts < 10000000000) ts *= 1000;
        double val = query.value(1).toDouble();
        if (val > 0) {
            dataList.append(qMakePair(ts, val));
        }
    }

    if (dataList.isEmpty()) {
        QMessageBox::information(this, "提示", "今天暂无测量数据");
        chartView->setChart(chart);
        return;
    }

    // ---- 创建序列 ----
    QLineSeries *series = new QLineSeries();
    series->setName(yTitle);
    series->setColor(Qt::red);

    for (const auto &p : dataList) {
        series->append(p.first, p.second);
    }

    // ---- 坐标轴 ----
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("hh:mm");
    axisX->setTitleText("测量时间");
    axisX->setRange(start, now);
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(yTitle);

    // 计算Y轴范围
    double minVal = 9999, maxVal = -9999;
    for (const auto &p : dataList) {
        if (p.second < minVal) minVal = p.second;
        if (p.second > maxVal) maxVal = p.second;
    }
    double padding = (maxVal - minVal) * 0.80;
    if (padding < 1.0) padding = 1.0;
    axisY->setRange(minVal - padding, maxVal + padding);
    chart->addAxis(axisY, Qt::AlignLeft);

    // ---- 添加序列 ----
    chart->addSeries(series);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    // ---- 应用主题 ----
    applyTheme();
    chartView->setChart(chart);
    chartView->chart()->zoomReset();
}