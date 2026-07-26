#ifndef PATIENTMONITOR_H
#define PATIENTMONITOR_H

#include <QWidget>
#include <QTimer>
#include <QLCDNumber>
#include <QLabel>
#include "wavewidget.h"

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