#include "ui/mainwindow.h"
#include "thememanager.h"
#include <QApplication>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <signal.h>

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    QFile file("medical_robot.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " ";
        switch (type) {
            case QtDebugMsg: out << "Debug: "; break;
            case QtInfoMsg:  out << "Info: "; break;
            case QtWarningMsg: out << "Warning: "; break;
            case QtCriticalMsg: out << "Critical: "; break;
            case QtFatalMsg: out << "Fatal: "; break;
        }
        out << msg << " (" << context.file << ":" << context.line << ")\n";
        file.close();
    }
    if (type == QtFatalMsg)
        abort();
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageHandler);
    QApplication a(argc, argv);
    
    // 应用默认浅色主题
    ThemeManager::instance().applyTheme(ThemeManager::Light);
    
    MainWindow w;
    w.show();
    return a.exec();
}