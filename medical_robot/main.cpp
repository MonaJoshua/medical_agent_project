#include "ui/mainwindow.h"
#include "thememanager.h"
#include <QApplication>
#include <QMutex>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

// 日志消息处理器（线程安全）
void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    QFile file("medical_robot.log");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        return;
    }

    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " ";

    switch (type) {
        case QtDebugMsg:    out << "Debug: "; break;
        case QtInfoMsg:     out << "Info: "; break;
        case QtWarningMsg:  out << "Warning: "; break;
        case QtCriticalMsg: out << "Critical: "; break;
        case QtFatalMsg:    out << "Fatal: "; break;
        default:            out << "Unknown: "; break;
    }

    out << msg << " (" << context.file << ":" << context.line << ")\n";
    file.close();

    if (type == QtFatalMsg) {
        abort();
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageHandler);
    QApplication app(argc, argv);

    // 应用浅色主题
    ThemeManager::instance().applyTheme(ThemeManager::Light);

    MainWindow window;
    window.show();

    return app.exec();
}