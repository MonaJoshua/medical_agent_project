#ifndef PATIENTMONITOR_H
#define PATIENTMONITOR_H

#include <QWidget>
#include <QLCDNumber>
#include <QLabel>
#include <QTimer>
#include <QPointer>
#include <QPushButton> 
#include "wavewidget.h" 

class TrendWidget;
struct VitalSigns;

struct AlarmThresholds {
    int hrMin = 60;
    int hrMax = 100;
    int spo2Min = 95;
    int spo2Max = 100;
    double tempMin = 36.0;
    double tempMax = 37.5;
};

class PatientMonitor : public QWidget
{
    Q_OBJECT
public:
    explicit PatientMonitor(QWidget *parent = nullptr);
    ~PatientMonitor();

signals:
    void requestMonitorDataPage();

private slots:
    void onDataReceived(const VitalSigns &data);
    void onStartMeasurementClicked();
    void onCollectTimerTimeout();
    void onHistoryClicked();
    void updateTheme();

private:
    void setupUI();
    QWidget* createValueCard(const QString &title, QLCDNumber *&lcd, QLabel *&timeLabel);
    void initDatabase();
    void flushBuffer();
    void finishMeasurement();
    void updateLastMeasurement();
    void showAlarmSettings();
    void showParameterTrend(const QString &paramName);

    WaveWidget *waveWidget; 
    QLCDNumber *hrLCD;
    QLCDNumber *spo2LCD;
    QLCDNumber *tempLCD;
    QLabel *m_hrTimeLabel;
    QLabel *m_spo2TimeLabel;
    QLabel *m_tempTimeLabel;
    QLabel *alarmLight;
    QPushButton *alarmSettingsBtn;
    QPushButton *startBtn; 

    QTimer *m_collectTimer;
    bool m_isCollecting;
    int m_collectInterval;
    QTimer *m_blinkTimer;
    QTimer *m_storeTimer;

    QList<VitalSigns> m_currentSessionData;
    QList<VitalSigns> m_dataBuffer;
    AlarmThresholds m_thresholds;

    QPointer<TrendWidget> m_trendWindow;
};

#endif // PATIENTMONITOR_H