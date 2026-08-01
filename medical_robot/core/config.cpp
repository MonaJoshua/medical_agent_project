#include "config.h"

Config& Config::instance()
{
    static Config instance;
    return instance;
}

Config::Config()
    : m_backendUrl("http://127.0.0.1:5000")
    , m_timeout(30)
{
}