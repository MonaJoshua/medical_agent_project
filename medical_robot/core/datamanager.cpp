#include "datamanager.h"
#include <QRandomGenerator>

DataManager& DataManager::instance()
{
    static DataManager instance;
    return instance;
}

DataManager::DataManager(QObject *parent)
    : QObject(parent)
{
    // 不再启动自动定时器
}

DataManager::~DataManager() {}

void DataManager::recordOnce()
{
    VitalSigns data = generateSimulatedData();
    data.timestamp = QDateTime::currentMSecsSinceEpoch();
    emit newDataAvailable(data);
}

VitalSigns DataManager::generateSimulatedData()
{
    VitalSigns data;
    static int hr = 75;
    // 每次生成时，心率在 60-100 之间缓慢变化
    hr += QRandomGenerator::global()->bounded(-2, 3);
    hr = qBound(60, hr, 100);
    data.heartRate = hr;
    data.spo2 = QRandomGenerator::global()->bounded(95, 101);
    data.temperature = 36.5 + QRandomGenerator::global()->bounded(-10, 15) / 10.0;
    data.timestamp = 0;  // 在 recordOnce 中设置
    return data;
}