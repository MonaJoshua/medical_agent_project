#ifndef SMARTTRIAGE_H
#define SMARTTRIAGE_H

#include <QWidget>
#include <QTextEdit>
#include <QTextBrowser>
#include <QNetworkAccessManager>
#include <QNetworkReply>

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

private:
    QTextEdit *symptomEdit;
    QTextBrowser *resultBrowser;
    QString currentTriageResult;
    QNetworkAccessManager *networkManager;
};

#endif // SMARTTRIAGE_H
