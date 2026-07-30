#include "ui/mainwindow.h"
#include "thememanager.h"
#include <QApplication>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <signal.h>

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    QFile file("medical_robot.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " ";
        switch (type) {
            case QtDebugMsg: out << "Debug: "; break;
            case QtInfoMsg:  out << "Info: "; break;
            case QtWarningMsg: out << "Warning: "; break;
            case QtCriticalMsg: out << "Critical: "; break;
            case QtFatalMsg: out << "Fatal: "; break;
        }
        out << msg << " (" << context.file << ":" << context.line << ")\n";
        file.close();
    }
    if (type == QtFatalMsg)
        abort();
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageHandler);
    QApplication a(argc, argv);
    
    // 应用默认浅色主题
    ThemeManager::instance().applyTheme(ThemeManager::Light);
    
// ============================================================
// 真实传感器数据读取（树莓派） 
// 当前为模拟占位，未来接入真实传感器时取消注释并实现
// ============================================================
/*
    #include <wiringPi.h>        // GPIO 库
    #include <wiringPiI2C.h>     // I2C 库（如 MAX30100 / MAX30102）
    #include <stdio.h>

    // 假设的传感器读取函数
    static VitalSigns readRealSensors()
    {
        VitalSigns data;
        data.timestamp = QDateTime::currentMSecsSinceEpoch();

        // TODO: 替换为真实传感器读取代码
        // 例如：
        // data.heartRate = readHeartRateSensor();
        // data.spo2 = readSPO2Sensor();
        // data.temperature = readTemperatureSensor();

        // 临时占位：返回安全默认值（避免界面空白）
        data.heartRate = 75;
        data.spo2 = 98;
        data.temperature = 36.5;

        return data;
    }

    static void initSensorHardware()
    {
        // 初始化 GPIO、I2C 等
        // wiringPiSetup();
        // 初始化传感器...
    }
    */

    MainWindow w;
    w.show();
    return a.exec();
}