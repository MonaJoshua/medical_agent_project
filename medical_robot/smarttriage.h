#ifndef SMARTTRIAGE_H
#define SMARTTRIAGE_H

#include <QWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTextEdit>
#include <QTextBrowser>

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
    void onRegisterReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    QNetworkAccessManager *registerManager;
    QString currentTriageResult;
    QTextEdit *symptomEdit;
    QTextBrowser *resultBrowser;
};

#endif // SMARTTRIAGE_H
