#ifndef PATIENTMONITOR_H
#define PATIENTMONITOR_H

#include <QWidget>
#include <QLCDNumber>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QPointer>
#include "wavewidget.h"
#include "core/datamanager.h"

class TrendWidget; 

class PatientMonitor : public QWidget
{
    Q_OBJECT   // 必须存在且正确

public:
    explicit PatientMonitor(QWidget *parent = nullptr);
    ~PatientMonitor();

    void updateTheme();

signals:
    void requestMonitorDataPage();

private slots:
    void onDataReceived(const VitalSigns &data);
    void showAlarmSettings();
    void showParameterTrend(const QString &paramName);
    void onStartMeasurementClicked();
    void onCollectTimerTimeout();
    void onHistoryClicked();

private:
    void setupUI();
    void initDatabase();
    void flushBuffer();
    QWidget* createValueCard(const QString &title, QLCDNumber *&lcd, QLabel *&timeLabel);
    void finishMeasurement();
    void updateLastMeasurement();

    // UI 组件
    WaveWidget *waveWidget;
    QLCDNumber *hrLCD;
    QLCDNumber *spo2LCD;
    QLCDNumber *tempLCD;
    QLabel *m_hrTimeLabel;
    QLabel *m_spo2TimeLabel;
    QLabel *m_tempTimeLabel;
    QLabel *alarmLight;
    QPushButton *emergencyBtn;
    QPushButton *alarmSettingsBtn;
    QPushButton *startBtn;
    QPointer<TrendWidget> m_trendWindow; 

    // 数据采集
    QTimer *m_collectTimer;
    bool m_isCollecting;
    int m_collectInterval;
    QList<VitalSigns> m_currentSessionData;

    // 报警
    struct AlarmThresholds {
        double hrMin = 40;
        double hrMax = 120;
        double spo2Min = 90;
        double spo2Max = 100;
        double tempMin = 35.0;
        double tempMax = 38.5;
    } m_thresholds;
    QTimer *m_blinkTimer;

    // 数据存储
    QTimer *m_storeTimer;
    QList<VitalSigns> m_dataBuffer;
    int m_maxBufferSize;

    bool m_isDark;
};

#endif // PATIENTMONITOR_H