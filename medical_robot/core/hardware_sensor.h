#ifndef HARDWARE_SENSOR_H
#define HARDWARE_SENSOR_H

#include "datamanager.h"
#include <QObject>

/**
 * @brief 树莓派硬件传感器读取模块
 * 
 * 当在树莓派上运行时，取消注释下方的 #define USE_REAL_SENSORS
 * 并在对应函数中实现真实的传感器读取代码。
 * 
 * 硬件方案：
 * - 心率/血氧：MAX30102（I2C）
 * - 体温：MLX90614（I2C）或 DS18B20（1-Wire）
 * 
 * 依赖库（树莓派上需要安装）：
 *   sudo apt-get install wiringpi libwiringpi-dev
 * 
 * 接线参考：
 *   MAX30102 VCC → 树莓派 3.3V
 *   MAX30102 GND → 树莓派 GND
 *   MAX30102 SCL → 树莓派 SCL（GPIO 3）
 *   MAX30102 SDA → 树莓派 SDA（GPIO 2）
 * 
 * 参考代码：
 *   https://github.com/adafruit/Adafruit_MAX30105
 *   https://github.com/Seeed-Studio/Seeed_MAX30102
 */
class HardwareSensor : public QObject
{
    Q_OBJECT
public:
    static HardwareSensor& instance();

    /**
     * @brief 初始化所有传感器硬件
     * @return true 初始化成功，false 失败
     */
    bool init();

    /**
     * @brief 读取一次真实传感器数据
     * @return VitalSigns 包含心率、血氧、体温的结构体
     */
    VitalSigns readOnce();

    /**
     * @brief 检查传感器是否已初始化
     */
    bool isReady() const { return m_ready; }

signals:
    void readyChanged(bool ready);

private:
    HardwareSensor();
    ~HardwareSensor();

    bool m_ready;

    // 传感器读取函数（需要根据实际硬件实现）
    double readHeartRate();
    double readSPO2();
    double readTemperature();

    // 硬件初始化函数
    bool initHeartRateSensor();
    bool initTemperatureSensor();
};

#endif // HARDWARE_SENSOR_H
