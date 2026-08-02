#include "datamanager.h"
#include "hardware_sensor.h"
#include <QRandomGenerator>
#include <QDateTime> 
DataManager& DataManager::instance()
{
    static DataManager instance;
    return instance;
}

DataManager::DataManager(QObject *parent)
    : QObject(parent)
{
    // 不启动自动定时器，由 PatientMonitor 手动触发
}

DataManager::~DataManager() {}

void DataManager::recordOnce()
{
    VitalSigns data = HardwareSensor::instance().isReady() 
                      ? readRealSensorData() 
                      : generateSimulatedData();
    data.timestamp = QDateTime::currentMSecsSinceEpoch();
    emit newDataAvailable(data);
}

VitalSigns DataManager::generateSimulatedData()
{
    VitalSigns data;
    
    // 心率随机游走（60-100 bpm）
    static int heartRate = 75;
    heartRate += QRandomGenerator::global()->bounded(-2, 3);
    heartRate = qBound(60, heartRate, 100);
    data.heartRate = heartRate;
    
    // 血氧 95-100%
    data.spo2 = QRandomGenerator::global()->bounded(95, 101);
    
    // 体温 36.0°C - 37.5°C（步长 0.1°C）
    const double tempStep = 0.1;
    int tempOffset = QRandomGenerator::global()->bounded(-10, 16);  // -1.0 ~ +1.5
    data.temperature = 36.5 + tempOffset * tempStep;
    
    // timestamp 由 recordOnce 设置，这里不赋值
    return data;
}

VitalSigns DataManager::readRealSensorData()
{
    return HardwareSensor::instance().readOnce();
}