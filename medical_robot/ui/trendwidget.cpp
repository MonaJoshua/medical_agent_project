// trendwidget.cpp
// 生理数据趋势图组件（基于 QtCharts）

#include "trendwidget.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QMessageBox>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>

TrendWidget::TrendWidget(const QString &paramName, QWidget *parent)
    : QWidget(parent)
    , m_paramName(paramName)
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

    chartView = new QChartView(this);
    chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(chartView);

    applyTheme();
}

void TrendWidget::applyTheme()
{
    bool isDark = (ThemeManager::instance().currentTheme() == ThemeManager::Dark);
    QChart *chart = chartView->chart();
    if (!chart) return;

    // 背景色
    chart->setBackgroundBrush(isDark ? QBrush(QColor(30, 30, 30)) : QBrush(QColor(255, 255, 255)));

    // 标题、图例、坐标轴统一使用蓝色
    QColor accentColor(66, 133, 244);
    chart->setTitleBrush(QBrush(accentColor));
    chart->legend()->setLabelColor(accentColor);

    for (QAbstractAxis *axis : chart->axes()) {
        axis->setLabelsColor(accentColor);
        axis->setTitleBrush(QBrush(accentColor));
    }
}

void TrendWidget::loadData()
{
    QChart *chart = new QChart();

    // 设置标题
    QString titleMap;
    if (m_paramName == "heartRate")    titleMap = "心率今日趋势";
    else if (m_paramName == "spo2")    titleMap = "血氧今日趋势";
    else if (m_paramName == "temperature") titleMap = "体温今日趋势";
    else titleMap = m_paramName + "今日趋势";
    chart->setTitle(titleMap);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    // 计算当天 00:00 到当前时间
    QDateTime now = QDateTime::currentDateTime();
    QDateTime start(now.date(), QTime(0, 0, 0));
    qint64 startMs = start.toMSecsSinceEpoch();

    // 检查数据库
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        QMessageBox::warning(this, "错误", "数据库未打开！");
        chartView->setChart(chart);
        return;
    }

    // 字段映射
    QString field, yTitle;
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

    // 查询数据
    QSqlQuery query;
    query.prepare(QString("SELECT timestamp, %1 FROM vital_data WHERE timestamp > ? ORDER BY timestamp ASC")
                  .arg(field));
    query.addBindValue(startMs);

    if (!query.exec()) {
        QMessageBox::warning(this, "错误", "查询失败：" + query.lastError().text());
        chartView->setChart(chart);
        return;
    }

    // 读取数据
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

    // 创建曲线序列
    QLineSeries *series = new QLineSeries();
    series->setName(yTitle);
    series->setColor(Qt::red);
    for (const auto &p : dataList) {
        series->append(p.first, p.second);
    }

    // X 轴（时间）
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("hh:mm");
    axisX->setTitleText("测量时间");
    axisX->setRange(start, now);
    chart->addAxis(axisX, Qt::AlignBottom);

    // Y 轴（数值）
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText(yTitle);

    // 计算 Y 轴范围（留 80% 边距）
    double minVal = dataList.first().second;
    double maxVal = dataList.first().second;
    for (const auto &p : dataList) {
        if (p.second < minVal) minVal = p.second;
        if (p.second > maxVal) maxVal = p.second;
    }
    double padding = (maxVal - minVal) * 0.8;
    if (padding < 1.0) padding = 1.0;
    axisY->setRange(minVal - padding, maxVal + padding);
    chart->addAxis(axisY, Qt::AlignLeft);

    // 关联序列和坐标轴
    chart->addSeries(series);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    // 应用主题并显示
    applyTheme();
    chartView->setChart(chart);
    chartView->chart()->zoomReset();
}