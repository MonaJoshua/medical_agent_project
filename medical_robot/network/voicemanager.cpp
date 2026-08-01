/**
*百度语音 STT（语音识别）和 TTS（语音合成）封装
*/
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

// 设置百度 API 密钥，并自动获取 access_token
void VoiceManager::setApiKeys(const QString &apiKey, const QString &secretKey)
{
    m_apiKey = apiKey;
    m_secretKey = secretKey;
    requestAccessToken();
}

// 向百度 OAuth 端点请求 access_token
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

// 处理 access_token 响应
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

// 从音频文件进行语音识别（16kHz WAV 格式）
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

// 发送 STT 请求（token 和 cuid 放在 JSON body 中）
void VoiceManager::sttRequest(const QByteArray &audioData)
{
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

// 处理 STT 响应
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

// TTS：将文本转为语音并播报
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

// 发送 TTS 请求
void VoiceManager::ttsRequest(const QString &text)
{
    QUrl url("https://tsn.baidu.com/text2audio");
    QUrlQuery query;
    query.addQueryItem("tex", text);
    query.addQueryItem("tok", m_accessToken);
    query.addQueryItem("cuid", "qt_medical_robot");
    query.addQueryItem("ctp", "1");
    query.addQueryItem("lan", "zh");
    query.addQueryItem("spd", "5");   // 语速
    query.addQueryItem("pit", "5");   // 音调
    query.addQueryItem("vol", "9");   // 音量
    query.addQueryItem("per", "4");   // 发音人（4 = 情感女声）
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, this, &VoiceManager::onTtsReply);
}

// 处理 TTS 响应
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

    // 有效音频以 0xFF 开头（MP3 头）
    if (audioData.size() > 0 && (unsigned char)audioData[0] == 0xFF) {
        playAudio(audioData);
    } else {
        QString errorMsg = QString::fromUtf8(audioData);
        emit speakError("合成失败: " + errorMsg);
    }

    reply->deleteLater();
}

// 保存音频为 MP3 并用系统默认播放器播放
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