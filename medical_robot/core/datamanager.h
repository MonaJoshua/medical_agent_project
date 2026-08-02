#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QObject>
#include <QDateTime>

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

    void recordOnce(); 

signals:
    void newDataAvailable(const VitalSigns &data);

private:
    DataManager(QObject *parent = nullptr);
    ~DataManager();

    VitalSigns generateSimulatedData();
    VitalSigns readRealSensorData(); 
};

#endif // DATAMANAGER_H