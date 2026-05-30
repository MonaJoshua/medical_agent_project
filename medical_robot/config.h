#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QSettings>

class Config
{
public:
    enum Mode { MODE_DEMO, MODE_PRODUCTION };
    
    static Config& instance() {
        static Config instance;
        return instance;
    }
    
    Mode getMode() const { return m_mode; }
    QString getModeString() const { return m_mode == MODE_DEMO ? "demo" : "production"; }
    
    // API 地址
    QString getBackendUrl() const { return m_backendUrl; }
    
    // 关怀模式
    bool isCareMode() const { return m_careMode; }
    void setCareMode(bool enable) { m_careMode = enable; }
    
private:
    Config();
    ~Config() = default;
    
    Mode m_mode;
    QString m_backendUrl;
    bool m_careMode;
};

#endif // CONFIG_H