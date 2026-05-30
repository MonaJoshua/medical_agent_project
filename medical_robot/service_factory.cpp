#include "service_factory.h"
#include "config.h"
#include "offline_triage.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>

ServiceFactory& ServiceFactory::instance()
{
    static ServiceFactory instance;
    return instance;
}

ServiceFactory::ServiceFactory()
    : m_networkManager(nullptr)
{
    m_mode = Config::instance().getModeString();
    m_backendUrl = Config::instance().getBackendUrl();
    m_networkManager = new QNetworkAccessManager(this);
    
    qDebug() << "🔧 服务工厂初始化 - 模式:" << m_mode 
             << "- 后端地址:" << m_backendUrl;
}

ServiceFactory::~ServiceFactory()
{
    if (m_networkManager) {
        delete m_networkManager;
        m_networkManager = nullptr;
    }
}

void ServiceFactory::triage(const QString& symptoms, 
                             std::function<void(bool, const QString&)> callback)
{
    if (m_mode == "demo") {
        // ========== 演示模式：使用离线分诊 ==========
        qDebug() << "🎮 [演示模式] 离线分诊 - 症状:" << symptoms;
        
        QString result = OfflineTriage::instance().triage(symptoms);
        
        // 模拟网络延迟（更真实的演示效果）
        QTimer::singleShot(500, [callback, result]() {
            callback(true, result);
        });
    } 
    else {
        // ========== 生产模式：调用后端 API ==========
        qDebug() << "🏥 [生产模式] 调用后端分诊 - 症状:" << symptoms;
        
        QUrl url(m_backendUrl + "/api/triage");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        
        QJsonObject jsonBody;
        jsonBody["symptoms"] = symptoms;
        
        QNetworkReply* reply = m_networkManager->post(
            request, 
            QJsonDocument(jsonBody).toJson()
        );
        
        // 使用 Lambda 捕获回调
        QObject::connect(reply, &QNetworkReply::finished, [reply, callback]() {
            if (reply->error() != QNetworkReply::NoError) {
                qDebug() << "❌ 分诊网络错误:" << reply->errorString();
                callback(false, reply->errorString());
                reply->deleteLater();
                return;
            }
            
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();
            
            if (obj["success"].toBool()) {
                QString result = obj["result"].toString();
                callback(true, result);
            } else {
                callback(false, obj["message"].toString());
            }
            
            reply->deleteLater();
        });
    }
}

void ServiceFactory::registerPatient(const QString& department,
                                      const QString& patientName,
                                      const QString& symptoms,
                                      std::function<void(bool, const QJsonObject&)> callback)
{
    if (m_mode == "demo") {
        // ========== 演示模式：模拟挂号响应 ==========
        qDebug() << "🎮 [演示模式] 模拟挂号 - 科室:" << department 
                 << "患者:" << patientName;
        
        // 模拟挂号成功响应
        static int demoQueueNumber = 0;
        demoQueueNumber++;
        
        QJsonObject mockResult;
        mockResult["success"] = true;
        mockResult["message"] = "挂号成功（演示模式）";
        mockResult["ticket_id"] = QString("DEMO%1").arg(demoQueueNumber, 4, 10, QChar('0'));
        mockResult["department"] = department;
        mockResult["queue_number"] = demoQueueNumber;
        mockResult["remaining_slots"] = 10 - demoQueueNumber;
        
        // 模拟网络延迟
        QTimer::singleShot(300, [callback, mockResult]() {
            callback(true, mockResult);
        });
    } 
    else {
        // ========== 生产模式：调用后端 API ==========
        qDebug() << "🏥 [生产模式] 调用后端挂号 - 科室:" << department;
        
        QUrl url(m_backendUrl + "/api/register");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        
        QJsonObject jsonBody;
        jsonBody["department"] = department;
        jsonBody["patient_name"] = patientName;
        jsonBody["symptoms"] = symptoms;
        
        QNetworkReply* reply = m_networkManager->post(
            request, 
            QJsonDocument(jsonBody).toJson()
        );
        
        QObject::connect(reply, &QNetworkReply::finished, [reply, callback]() {
            if (reply->error() != QNetworkReply::NoError) {
                qDebug() << "❌ 挂号网络错误:" << reply->errorString();
                
                QJsonObject errorResult;
                errorResult["success"] = false;
                errorResult["message"] = reply->errorString();
                callback(false, errorResult);
                reply->deleteLater();
                return;
            }
            
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();
            
            callback(obj["success"].toBool(), obj);
            reply->deleteLater();
        });
    }
}

QString ServiceFactory::getBackendUrl() const
{
    return m_backendUrl;
}

// 静态辅助方法：从分诊结果中提取科室名
QString ServiceFactory::extractDepartment(const QString& triageResult)
{
    if (triageResult.contains("神经内科")) return "神经内科";
    if (triageResult.contains("心内科")) return "心内科";
    if (triageResult.contains("呼吸内科")) return "呼吸内科";
    if (triageResult.contains("消化内科")) return "消化内科";
    if (triageResult.contains("内科")) return "内科";
    if (triageResult.contains("外科")) return "外科";
    if (triageResult.contains("骨科")) return "骨科";
    if (triageResult.contains("儿科")) return "儿科";
    if (triageResult.contains("眼科")) return "眼科";
    if (triageResult.contains("口腔科")) return "口腔科";
    if (triageResult.contains("皮肤科")) return "皮肤科";
    return "全科门诊";
}