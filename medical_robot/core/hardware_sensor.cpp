#include "hardware_sensor.h"
#include <QDebug>
#include <QDateTime>
#include <QRandomGenerator> 
#include <QtMath>

// ============================================================
// 树莓派硬件传感器配置开关
// 在树莓派上运行时，取消下方注释以启用真实传感器
// ============================================================
// #define USE_REAL_SENSORS

#ifdef USE_REAL_SENSORS
    #include <wiringPi.h>
    #include <wiringPiI2C.h>
    // 如果使用 MAX30102，需要额外添加驱动库
    // #include "max30102.h"
#endif

HardwareSensor& HardwareSensor::instance()
{
    static HardwareSensor instance;
    return instance;
}

HardwareSensor::HardwareSensor()
    : m_ready(false)
{
}

HardwareSensor::~HardwareSensor()
{
}

bool HardwareSensor::init()
{
#ifdef USE_REAL_SENSORS
    qDebug() << "[HardwareSensor] 正在初始化树莓派传感器...";

    // 1. 初始化 WiringPi
    if (wiringPiSetup() == -1) {
        qDebug() << "[HardwareSensor] ❌ wiringPiSetup 失败";
        return false;
    }

    // 2. 初始化心率/血氧传感器
    if (!initHeartRateSensor()) {
        qDebug() << "[HardwareSensor] ❌ 心率/血氧传感器初始化失败";
        return false;
    }

    // 3. 初始化体温传感器
    if (!initTemperatureSensor()) {
        qDebug() << "[HardwareSensor] ❌ 体温传感器初始化失败";
        return false;
    }

    m_ready = true;
    qDebug() << "[HardwareSensor] ✅ 所有传感器初始化成功";
    return true;
#else
    qDebug() << "[HardwareSensor] ⚠️ 模拟模式：未启用真实传感器（请在 hardware_sensor.cpp 中取消注释 #define USE_REAL_SENSORS）";
    m_ready = false;
    return false;
#endif
}

bool HardwareSensor::initHeartRateSensor()
{
#ifdef USE_REAL_SENSORS
    // TODO: 初始化 MAX30102
    // 示例代码：
    // int fd = wiringPiI2CSetup(0x57);  // MAX30102 默认 I2C 地址
    // if (fd < 0) return false;
    // 配置寄存器...
    qDebug() << "[HardwareSensor] ⚠️ initHeartRateSensor() 需要实现";
    return false;
#else
    return false;
#endif
}

bool HardwareSensor::initTemperatureSensor()
{
#ifdef USE_REAL_SENSORS
    // TODO: 初始化 MLX90614 或 DS18B20
    // 示例代码（MLX90614）：
    // int fd = wiringPiI2CSetup(0x5A);  // MLX90614 默认 I2C 地址
    // if (fd < 0) return false;
    qDebug() << "[HardwareSensor] ⚠️ initTemperatureSensor() 需要实现";
    return false;
#else
    return false;
#endif
}

double HardwareSensor::readHeartRate()
{
#ifdef USE_REAL_SENSORS
    // TODO: 从 MAX30102 读取心率
    // 返回 bpm 值
    qDebug() << "[HardwareSensor] ⚠️ readHeartRate() 需要实现";
    return 75.0;  // 默认占位值
#else
    return 75.0;
#endif
}

double HardwareSensor::readSPO2()
{
#ifdef USE_REAL_SENSORS
    // TODO: 从 MAX30102 读取血氧
    qDebug() << "[HardwareSensor] ⚠️ readSPO2() 需要实现";
    return 98.0;
#else
    return 98.0;
#endif
}

double HardwareSensor::readTemperature()
{
#ifdef USE_REAL_SENSORS
    // TODO: 从 MLX90614 或 DS18B20 读取体温
    qDebug() << "[HardwareSensor] ⚠️ readTemperature() 需要实现";
    return 36.5;
#else
    return 36.5;
#endif
}

VitalSigns HardwareSensor::readOnce()
{
    VitalSigns data;
    data.timestamp = QDateTime::currentMSecsSinceEpoch();

    if (m_ready) {
        // 读取真实传感器数据
        data.heartRate = readHeartRate();
        data.spo2 = readSPO2();
        data.temperature = readTemperature();
    } else {
        // 模拟数据（本地生成，不依赖 DataManager）
        static int hr = 75;
        hr += QRandomGenerator::global()->bounded(-2, 3);
        hr = qBound(60, hr, 100);
        data.heartRate = hr;
        data.spo2 = QRandomGenerator::global()->bounded(95, 101);
        data.temperature = 36.5 + QRandomGenerator::global()->bounded(-10, 15) / 10.0;
    }
    return data;
}