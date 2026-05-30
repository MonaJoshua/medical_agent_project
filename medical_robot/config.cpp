#include "config.h"
#include <QDebug>

Config::Config()
{
    // 从环境变量读取模式，默认 demo
    QString modeStr = QString::fromLocal8Bit(qgetenv("MEDICAL_MODE"));
    if (modeStr == "production") {
        m_mode = MODE_PRODUCTION;
        qDebug() << "🏥 运行模式: 生产模式";
    } else {
        m_mode = MODE_DEMO;
        qDebug() << "🎮 运行模式: 演示模式";
    }
    
    // 后端地址
    m_backendUrl = "http://127.0.0.1:5000";
    
    // 关怀模式（可从配置文件读取）
    m_careMode = false;
}