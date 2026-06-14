#include "voicemanager.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

VoiceManager::VoiceManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

VoiceManager::~VoiceManager()
{
}

void VoiceManager::setApiKeys(const QString &apiKey, const QString &secretKey)
{
    m_apiKey = apiKey;
    m_secretKey = secretKey;
    requestAccessToken();
}

void VoiceManager::requestAccessToken()
{
    QUrl url("https://aip.baidubce.com/oauth/2.0/token");
    QUrlQuery query;
    query.addQueryItem("grant_type", "client_credentials");
    query.addQueryItem("client_id", m_apiKey);
    query.addQueryItem("client_secret", m_secretKey);
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, &VoiceManager::onAccessTokenReply);
}

void VoiceManager::onAccessTokenReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "获取 access_token 失败:" << reply->errorString();
        emit recognitionError("获取 access_token 失败: " + reply->errorString());
        reply->deleteLater();
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    m_accessToken = doc.object()["access_token"].toString();
    
    qDebug() << "✅ access_token 获取成功";
    emit accessTokenReady();
    reply->deleteLater();
}

void VoiceManager::recognizeFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit recognitionError("无法打开音频文件: " + filePath);
        return;
    }
    
    QByteArray audioData = file.readAll();
    file.close();
    
    if (m_accessToken.isEmpty()) {
        connect(this, &VoiceManager::accessTokenReady, [this, audioData]() {
            sttRequest(audioData);
        });
        requestAccessToken();
    } else {
        sttRequest(audioData);
    }
}

void VoiceManager::sttRequest(const QByteArray &audioData)
{
    // 关键：token 和 cuid 放在 JSON body 中，不是 URL 参数
    QUrl url("https://vop.baidu.com/server_api");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QString base64Audio = QString::fromLatin1(audioData.toBase64());
    
    QJsonObject json;
    json["format"] = "wav";
    json["rate"] = 16000;
    json["channel"] = 1;
    json["cuid"] = "qt_medical_robot";
    json["token"] = m_accessToken;
    json["speech"] = base64Audio;
    json["len"] = audioData.size();
    
    qDebug() << "发送语音识别请求，音频大小:" << audioData.size() << "bytes";
    
    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, &VoiceManager::onSttReply);
}

void VoiceManager::onSttReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    if (reply->error() != QNetworkReply::NoError) {
        emit recognitionError("STT API 错误: " + reply->errorString());
        reply->deleteLater();
        return;
    }
    
    QByteArray data = reply->readAll();
    qDebug() << "STT 响应:" << data;
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    
    int errNo = obj["err_no"].toInt();
    QString errMsg = obj["err_msg"].toString();
    
    if (errNo == 0 && errMsg == "success.") {
        QJsonArray resultArray = obj["result"].toArray();
        if (!resultArray.isEmpty()) {
            QString result = resultArray[0].toString();
            emit recognitionFinished(result);
        } else {
            emit recognitionError("未识别到文字");
        }
    } else {
        emit recognitionError("识别失败: " + errMsg);
    }
    
    reply->deleteLater();
}

void VoiceManager::speak(const QString &text)
{
    if (m_accessToken.isEmpty()) {
        connect(this, &VoiceManager::accessTokenReady, [this, text]() {
            ttsRequest(text);
        });
        requestAccessToken();
    } else {
        ttsRequest(text);
    }
}

void VoiceManager::ttsRequest(const QString &text)
{
    QUrl url("https://tsn.baidu.com/text2audio");
    QUrlQuery query;
    query.addQueryItem("tex", text);
    query.addQueryItem("tok", m_accessToken);
    query.addQueryItem("cuid", "qt_medical_robot");
    query.addQueryItem("ctp", "1");
    query.addQueryItem("lan", "zh");
    query.addQueryItem("spd", "5");
    query.addQueryItem("pit", "5");
    query.addQueryItem("vol", "9");
    query.addQueryItem("per", "4");
    url.setQuery(query);
    
    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, &VoiceManager::onTtsReply);
}

void VoiceManager::onTtsReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    if (reply->error() != QNetworkReply::NoError) {
        emit speakError("TTS API 错误: " + reply->errorString());
        reply->deleteLater();
        return;
    }
    
    QByteArray audioData = reply->readAll();
    
    if (audioData.size() > 0 && (unsigned char)audioData[0] == 0xFF) {
        playAudio(audioData);
    } else {
        QString errorMsg = QString::fromUtf8(audioData);
        emit speakError("合成失败: " + errorMsg);
    }
    
    reply->deleteLater();
}

void VoiceManager::playAudio(const QByteArray &audioData)
{
    QString tempFile = "/tmp/tts_output.mp3";
    QFile file(tempFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(audioData);
        file.close();
        qDebug() << "🔊 音频已保存到:" << tempFile;
        QProcess::startDetached("xdg-open", QStringList() << tempFile);
    }
    emit speakFinished();
}
