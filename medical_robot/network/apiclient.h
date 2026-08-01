#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <functional>
#include <QJsonObject>

//后端 API 客户端（单例）
class ApiClient : public QObject
{
    Q_OBJECT
public:
    static ApiClient& instance();

    void setBackendUrl(const QString& url);

    void triage(const QString& symptoms,
                std::function<void(bool, const QString&, const QString&)> callback,
                int retryCount = 1);

    void registerPatient(const QString& department,
                         const QString& patientName,
                         const QString& symptoms,
                         std::function<void(bool, const QJsonObject&)> callback,
                         int retryCount = 1);

    void summarizeSymptom(const QString& symptom,
                          std::function<void(bool, const QString&)> callback);

private:
    ApiClient();
    ~ApiClient();

    void postWithTimeout(
        const QString& endpoint,
        const QJsonObject& jsonBody,
        int timeout,
        std::function<void(const QJsonObject&)> onSuccess,
        std::function<void(const QString&)> onError);

    QNetworkAccessManager* m_manager;
    QString m_backendUrl;
};

#endif // APICLIENT_H