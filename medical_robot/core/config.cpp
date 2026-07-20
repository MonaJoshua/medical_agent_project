#include "config.h"
#include <cstdio>

Config& Config::instance()
{
    static Config instance;
    return instance;
}

Config::Config()
{
    // 默认生产模式（联网）
    m_mode = MODE_PRODUCTION;
    m_backendUrl = "http://127.0.0.1:5000";
}