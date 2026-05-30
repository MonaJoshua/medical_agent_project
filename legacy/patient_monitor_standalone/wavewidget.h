#ifndef WAVEWIDGET_H
#define WAVEWIDGET_H

#include <QWidget>
#include <QVector>

class WaveWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WaveWidget(QWidget *parent = nullptr);
    void addDataPoint(double value);   // 添加新数据点

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> dataPoints;        // 存储数据点
    int maxPoints = 200;               // 最多200个点
};

#endif // WAVEWIDGET_H
