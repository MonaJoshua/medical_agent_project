#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config
{
public:
    static Config& instance();

    // 后端服务地址
    QString getBackendUrl() const { return m_backendUrl; }
    void setBackendUrl(const QString& url) { m_backendUrl = url; }

    // 网络超时时间（秒）
    int getTimeout() const { return m_timeout; }
    void setTimeout(int timeout) { m_timeout = timeout; }

private:
    Config();
    ~Config() = default;

    // 禁止拷贝（确保单例）
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    QString m_backendUrl;
    int m_timeout;
};

#endif // CONFIG_H