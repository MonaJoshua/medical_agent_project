#ifndef TRENDWIDGET_H
#define TRENDWIDGET_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

QT_CHARTS_USE_NAMESPACE

class TrendWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TrendWidget(const QString &paramName, QWidget *parent = nullptr);
    ~TrendWidget();

private:
    void setupUI();
    void loadData();
    void applyTheme();

    QChartView *chartView;
    QString m_paramName;
};

#endif // TRENDWIDGET_H