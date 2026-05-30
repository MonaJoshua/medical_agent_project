#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTextEdit>
#include <QTextBrowser>
#include <QPushButton>

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
    void onRegisterReplyFinished(QNetworkReply *reply);  // 新增：挂号响应处理

private:
    QNetworkAccessManager *networkManager;
    QNetworkAccessManager *registerManager;  // 新增：挂号专用网络管理器
    QString currentTriageResult;
    QTextEdit *symptomEdit;
    QTextBrowser *resultBrowser;
    QPushButton *analyzeBtn;
    QPushButton *registerBtn;
};

#endif // MAINWINDOW_H
