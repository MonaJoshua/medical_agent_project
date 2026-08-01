#ifndef WAVEWIDGET_H
#define WAVEWIDGET_H

#include <QWidget>
#include <QVector>

class WaveWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WaveWidget(QWidget *parent = nullptr);
    void addDataPoint(double value);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> dataPoints;
    const int maxPoints = 100;
};

#endif // WAVEWIDGET_H
