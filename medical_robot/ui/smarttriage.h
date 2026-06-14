#ifndef SMARTTRIAGE_H
#define SMARTTRIAGE_H

#include <QWidget>
#include <QTextEdit>
#include <QTextBrowser>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class VoiceManager;

class SmartTriage : public QWidget
{
    Q_OBJECT

public:
    explicit SmartTriage(QWidget *parent = nullptr);
    ~SmartTriage();

private slots:
    void onAnalyzeClicked();
    void onRegisterClicked();
    void onReplyFinished(QNetworkReply *reply);
    void onVoiceInputClicked();
    void onVoiceRecognized(const QString &text);

private:
    QTextEdit *symptomEdit;
    QTextBrowser *resultBrowser;
    QString currentTriageResult;
    QNetworkAccessManager *networkManager;
    VoiceManager *voiceManager;
};

#endif // SMARTTRIAGE_H
