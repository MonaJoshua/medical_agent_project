#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config
{
public:
    static Config& instance();
    
    enum Mode { MODE_DEMO, MODE_PRODUCTION };
    Mode getMode() const { return m_mode; }
    QString getModeString() const { return m_mode == MODE_DEMO ? "demo" : "production"; }
    QString getBackendUrl() const { return m_backendUrl; }
    
    void setMode(Mode mode) { m_mode = mode; }

private:
    Config();
    ~Config() = default;
    
    Mode m_mode;
    QString m_backendUrl;
};

#endif // CONFIG_H
