#include "config.h"
#include <QDebug>
#include <cstdlib>

Config& Config::instance()
{
    static Config instance;
    return instance;
}

Config::Config()
{
    const char* env = std::getenv("MEDICAL_MODE");
    if (env && QString(env) == "production") {
        m_mode = MODE_PRODUCTION;
        qDebug() << "🏥 运行模式: 生产模式 (production)";
    } else {
        m_mode = MODE_DEMO;
        qDebug() << "🎮 运行模式: 演示模式 (demo)";
    }
    m_backendUrl = "http://127.0.0.1:5000";
}
