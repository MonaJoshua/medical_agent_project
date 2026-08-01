#ifndef VOICEMANAGER_H
#define VOICEMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QProcess>

// 百度语音 STT/TTS 管理器
class VoiceManager : public QObject
{
    Q_OBJECT
public:
    explicit VoiceManager(QObject *parent = nullptr);
    ~VoiceManager();

    void setApiKeys(const QString &apiKey, const QString &secretKey);
    void recognizeFromFile(const QString &filePath); 
    void speak(const QString &text); 

signals:
    void recognitionFinished(const QString &text);
    void recognitionError(const QString &error);
    void speakFinished();
    void speakError(const QString &error);
    void accessTokenReady();

private slots:
    void onAccessTokenReply();
    void onSttReply();
    void onTtsReply();

private:
    void requestAccessToken();
    void sttRequest(const QByteArray &audioData);
    void ttsRequest(const QString &text);
    void playAudio(const QByteArray &audioData);

    QNetworkAccessManager *m_networkManager;
    QString m_accessToken;
    QString m_apiKey;
    QString m_secretKey;
};

#endif // VOICEMANAGER_H