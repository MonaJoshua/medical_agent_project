#ifndef PATIENTMONITOR_H
#define PATIENTMONITOR_H

#include <QWidget>
#include <QTimer>
#include <QLCDNumber>
#include <QLabel>

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
    int maxPoints = 200;
};

class PatientMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit PatientMonitor(QWidget *parent = nullptr);
    ~PatientMonitor();

private slots:
    void updateData();

private:
    QTimer *timer;
    WaveWidget *waveWidget;
    QLCDNumber *hrLCD;
    QLCDNumber *spo2LCD;
    QLCDNumber *tempLCD;
    QLabel *alarmLight;
};

#endif // PATIENTMONITOR_H
