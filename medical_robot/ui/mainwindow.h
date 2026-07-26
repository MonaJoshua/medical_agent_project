#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCloseEvent>

class VoiceChatWidget;
class SmartTriage;
class PatientMonitor;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void toggleDarkMode(bool checked);   // 深色模式切换
    void showWindow();
    void quitApp();
    void switchToMonitor();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void createTrayIcon();

    QSystemTrayIcon *trayIcon;
    QMenu *trayMenu;

    bool isOfflineMode;             // 保留但不再使用
    SmartTriage *smartTriage;
    VoiceChatWidget *voiceChat;
    PatientMonitor *patientMonitor;

    // 深色模式相关
    QPushButton *darkModeBtn;
    bool m_isDarkMode;
};

#endif // MAINWINDOW_H