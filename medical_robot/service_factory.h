#ifndef SERVICE_FACTORY_H
#define SERVICE_FACTORY_H

#include <QString>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <functional>
#include <QTimer>

class ServiceFactory : public QObject
{
    Q_OBJECT

public:
    static ServiceFactory& instance();
    
    // 分诊接口
    void triage(const QString& symptoms, 
                std::function<void(bool success, const QString& result)> callback);
    
    // 挂号接口
    void registerPatient(const QString& department,
                         const QString& patientName,
                         const QString& symptoms,
                         std::function<void(bool success, const QJsonObject& result)> callback);
    
    // 获取后端 URL
    QString getBackendUrl() const;
    
    // 工具方法：从分诊结果提取科室名
    static QString extractDepartment(const QString& triageResult);
    
private:
    ServiceFactory();
    ~ServiceFactory();
    
    QString m_mode;
    QString m_backendUrl;
    QNetworkAccessManager* m_networkManager;
};

#endif // SERVICE_FACTORY_H