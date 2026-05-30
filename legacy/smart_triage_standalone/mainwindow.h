#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTextEdit>
#include <QTextBrowser>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAnalyzeClicked();
    void onRegisterClicked();
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *networkManager;
    QString currentTriageResult;
    QTextEdit *symptomEdit;
    QTextBrowser *resultBrowser;
};

#endif // MAINWINDOW_H
