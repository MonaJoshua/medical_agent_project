#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QObject>
#include <QDateTime>
#include <QTimer>
#include <functional>

struct VitalSigns {
    qint64 timestamp;
    double heartRate;
    double spo2;
    double temperature;
};

class DataManager : public QObject
{
    Q_OBJECT
public:
    static DataManager& instance();

    // 不再需要 setDataSource 和 start/stop
    void recordOnce();   // 手动触发一次数据记录

signals:
    void newDataAvailable(const VitalSigns &data);

private:
    DataManager(QObject *parent = nullptr);
    ~DataManager();

    VitalSigns generateSimulatedData();   // 生成一次有效数据

private:
    // 移除 m_timer 和 m_dataSource
};

#endif // DATAMANAGER_H