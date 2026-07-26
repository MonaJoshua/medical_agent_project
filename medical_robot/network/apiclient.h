#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <functional>
#include <QJsonObject>

class ApiClient : public QObject
{
    Q_OBJECT
public:
    static ApiClient& instance();
    
    void setBackendUrl(const QString& url);
    
    // 带重试次数的分诊（内部自动重试）
    void triage(const QString& symptoms, std::function<void(bool, const QString&)> callback, int retryCount = 1);
    
    void registerPatient(const QString& department, const QString& patientName,
                         const QString& symptoms, std::function<void(bool, const QJsonObject&)> callback, int retryCount = 1);

    void sendSymptomRecord(const QString& patientName, const QString& symptom,
                        const QString& department, const QString& suggestion);
                        
    void summarizeSymptom(const QString& symptom, 
                        std::function<void(bool, const QString&)> callback);

private:
    ApiClient();
    ~ApiClient();
    QNetworkAccessManager* m_manager;
    QString m_backendUrl;
};

#endif // APICLIENT_H
