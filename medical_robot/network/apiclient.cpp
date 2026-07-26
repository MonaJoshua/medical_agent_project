#include "apiclient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

ApiClient& ApiClient::instance()
{
    static ApiClient instance;
    return instance;
}

ApiClient::ApiClient()
    : m_manager(new QNetworkAccessManager(this))
    , m_backendUrl("http://127.0.0.1:5000")
{
}

ApiClient::~ApiClient() {}

void ApiClient::setBackendUrl(const QString& url)
{
    m_backendUrl = url;
}

void ApiClient::triage(const QString& symptoms, std::function<void(bool, const QString&)> callback, int retryCount)
{
    QUrl url(m_backendUrl + "/api/triage");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(30000);  // 30秒超时

    QJsonObject jsonBody;
    jsonBody["symptoms"] = symptoms;

    QNetworkReply* reply = m_manager->post(request, QJsonDocument(jsonBody).toJson());
    
    // 超时定时器
    QTimer* timer = new QTimer;
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [reply, timer, symptoms, callback, retryCount]() {
        if (reply && reply->isRunning()) {
            reply->abort();
            if (retryCount > 0) {
                qDebug() << "⚠️ 分诊请求超时，剩余重试次数:" << retryCount;
                ApiClient::instance().triage(symptoms, callback, retryCount - 1);
            } else {
                callback(false, "网络超时，请稍后重试");
            }
        }
        timer->deleteLater();
    });
    timer->start(30000);

    connect(reply, &QNetworkReply::finished, [reply, timer, callback]() {
        timer->stop();
        timer->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QString err = QString("网络错误: %1").arg(reply->errorString());
            callback(false, err);
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();
            if (obj["success"].toBool()) {
                callback(true, obj["result"].toString());
            } else {
                callback(false, obj["message"].toString());
            }
        }
        reply->deleteLater();
    });
}

void ApiClient::registerPatient(const QString& department, const QString& patientName,
                                const QString& symptoms, std::function<void(bool, const QJsonObject&)> callback,
                                int retryCount)
{
    QUrl url(m_backendUrl + "/api/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(30000);

    QJsonObject jsonBody;
    jsonBody["department"] = department;
    jsonBody["patient_name"] = patientName;
    jsonBody["symptoms"] = symptoms;

    QNetworkReply* reply = m_manager->post(request, QJsonDocument(jsonBody).toJson());
    
    QTimer* timer = new QTimer;
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [reply, timer, department, patientName, symptoms, callback, retryCount]() {
        if (reply && reply->isRunning()) {
            reply->abort();
            if (retryCount > 0) {
                qDebug() << "⚠️ 挂号请求超时，剩余重试次数:" << retryCount;
                ApiClient::instance().registerPatient(department, patientName, symptoms, callback, retryCount - 1);
            } else {
                QJsonObject err;
                err["success"] = false;
                err["message"] = "挂号超时，请稍后重试";
                callback(false, err);
            }
        }
        timer->deleteLater();
    });
    timer->start(30000);

    connect(reply, &QNetworkReply::finished, [reply, timer, callback]() {
        timer->stop();
        timer->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QJsonObject err;
            err["success"] = false;
            err["message"] = reply->errorString();
            callback(false, err);
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();
            callback(obj["success"].toBool(), obj);
        }
        reply->deleteLater();
    });
}

void ApiClient::summarizeSymptom(const QString& symptom,
                                 std::function<void(bool, const QString&)> callback)
{
    QUrl url(m_backendUrl + "/api/summarize");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject obj;
    obj["symptom"] = symptom;
    QNetworkReply* reply = m_manager->post(request, QJsonDocument(obj).toJson());

    QTimer* timer = new QTimer;
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [reply, timer, callback]() {
        if (reply && reply->isRunning()) {
            reply->abort();
            callback(false, "摘要请求超时");
        }
        timer->deleteLater();
    });
    timer->start(15000);  // 15秒超时

    connect(reply, &QNetworkReply::finished, [reply, timer, callback]() {
        timer->stop();
        timer->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            callback(false, reply->errorString());
        } else {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();
            if (obj["success"].toBool()) {
                callback(true, obj["summary"].toString());
            } else {
                callback(false, obj["message"].toString());
            }
        }
        reply->deleteLater();
    });
}

void ApiClient::sendSymptomRecord(const QString& patientName, const QString& symptom,
                                  const QString& department, const QString& suggestion)
{
    QUrl url(m_backendUrl + "/api/symptom_records");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 若需API Key，可在此设置，例如：
    // request.setRawHeader("X-API-Key", "your_api_key_here");

    QJsonObject obj;
    obj["patient_name"] = patientName;
    obj["symptom"] = symptom;
    obj["department"] = department;
    obj["suggestion"] = suggestion;

    QNetworkReply* reply = m_manager->post(request, QJsonDocument(obj).toJson());
    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "症状记录发送失败:" << reply->errorString();
        }
        reply->deleteLater();
    });
}