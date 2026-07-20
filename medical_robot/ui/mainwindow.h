#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>
#include <QPushButton>

class SmartTriage;  // 前向声明

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void showWindow();
    void quitApp();
    void toggleMode();
    void toggleTriageMode();

private:
    void createTrayIcon();
    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;
    QPushButton *modeBtn;
    bool isOfflineMode;
    QPushButton *triageModeBtn;
    bool isMultiRoundMode;
    SmartTriage *smartTriage;  // 保存智能导诊指针
};

#endif // MAINWINDOW_H