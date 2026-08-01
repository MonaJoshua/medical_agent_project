/**
*后端 API 客户端实现
*封装与 Flask 后端的所有 HTTP 通信，支持超时重试。
*所有方法均为异步，通过回调函数返回结果。
*/
#include "apiclient.h"
#include "config.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

// ============================================================
// 单例实现
// ============================================================
ApiClient& ApiClient::instance()
{
    static ApiClient instance;
    return instance;
}

ApiClient::ApiClient()
    : m_manager(new QNetworkAccessManager(this))
    , m_backendUrl("http://127.0.0.1:5000")
{
    // 从配置文件读取后端地址
    m_backendUrl = Config::instance().getBackendUrl();
}

ApiClient::~ApiClient() {}

void ApiClient::setBackendUrl(const QString& url)
{
    m_backendUrl = url;
}

// ============================================================
// 私有辅助：发起带超时的 POST 请求
// ============================================================
void ApiClient::postWithTimeout(
    const QString& endpoint,
    const QJsonObject& jsonBody,
    int timeout,
    std::function<void(const QJsonObject&)> onSuccess,
    std::function<void(const QString&)> onError)
{
    QUrl url(m_backendUrl + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(timeout);

    QNetworkReply* reply = m_manager->post(request, QJsonDocument(jsonBody).toJson());

    // 超时处理：使用单次定时器
    QTimer* timer = new QTimer;
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [reply, timer, onError]() {
        if (reply && reply->isRunning()) {
            reply->abort();
            onError("请求超时，请稍后重试");
        }
        timer->deleteLater();
    });
    timer->start(timeout);

    // 响应处理
    connect(reply, &QNetworkReply::finished, [reply, timer, onSuccess, onError]() {
        timer->stop();
        timer->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            onError(QString("网络错误: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        reply->deleteLater();

        if (!doc.isObject()) {
            onError("响应格式错误");
            return;
        }

        onSuccess(doc.object());
    });
}

// ============================================================
// 公开 API 方法
// ============================================================
// AI 分诊：症状 → 科室 + 建议
void ApiClient::triage(const QString& symptoms, 
                       std::function<void(bool, const QString&, const QString&)> callback,
                       int retryCount)
{
    int timeout = Config::instance().getTimeout() * 1000;  // 转毫秒

    QJsonObject jsonBody;
    jsonBody["symptoms"] = symptoms;

    postWithTimeout("/api/triage", jsonBody, timeout,
        // 成功回调
        [callback](const QJsonObject& resp) {
            if (resp["success"].toBool()) {
                QString department = resp["department"].toString("全科门诊");
                QString suggestion = resp["suggestion"].toString("请及时就医");
                callback(true, department, suggestion);
            } else {
                callback(false, "", resp["message"].toString("分诊失败"));
            }
        },
        // 失败回调（含重试）
        [this, symptoms, callback, retryCount](const QString& error) {
            if (retryCount > 0) {
                qDebug() << "[ApiClient] 分诊失败，剩余重试次数:" << retryCount << "，错误:" << error;
                triage(symptoms, callback, retryCount - 1);
            } else {
                callback(false, "", error);
            }
        }
    );
}

// 挂号提交
void ApiClient::registerPatient(const QString& department, 
                                const QString& patientName,
                                const QString& symptoms,
                                std::function<void(bool, const QJsonObject&)> callback,
                                int retryCount)
{
    int timeout = Config::instance().getTimeout() * 1000;

    QJsonObject jsonBody;
    jsonBody["department"] = department;
    jsonBody["patient_name"] = patientName;
    jsonBody["symptoms"] = symptoms;

    postWithTimeout("/api/register", jsonBody, timeout,
        // 成功回调
        [callback](const QJsonObject& resp) {
            bool ok = resp["success"].toBool();
            callback(ok, resp);
        },
        // 失败回调（含重试）
        [this, department, patientName, symptoms, callback, retryCount](const QString& error) {
            if (retryCount > 0) {
                qDebug() << "[ApiClient] 挂号失败，剩余重试次数:" << retryCount << "，错误:" << error;
                registerPatient(department, patientName, symptoms, callback, retryCount - 1);
            } else {
                QJsonObject err;
                err["success"] = false;
                err["message"] = error;
                callback(false, err);
            }
        }
    );
}

// 症状摘要：将长症状描述压缩为 30 字以内
void ApiClient::summarizeSymptom(const QString& symptom,
                                 std::function<void(bool, const QString&)> callback)
{
    int timeout = Config::instance().getTimeout() * 1000;

    QJsonObject jsonBody;
    jsonBody["symptom"] = symptom;

    postWithTimeout("/api/summarize", jsonBody, timeout,
        // 成功回调
        [callback](const QJsonObject& resp) {
            if (resp["success"].toBool()) {
                callback(true, resp["summary"].toString());
            } else {
                callback(false, resp["message"].toString("摘要生成失败"));
            }
        },
        // 失败回调（摘要失败时降级为本地截断）
        [callback, symptom](const QString& error) {
            qDebug() << "[ApiClient] 摘要请求失败:" << error << "，降级为本地截断";
            // 降级方案：本地截断
            QString fallback = symptom;
            if (fallback.length() > 30) {
                fallback = fallback.left(30) + "...";
            }
            callback(true, fallback);  // 仍返回 success=true，让调用方继续流程
        }
    );
}