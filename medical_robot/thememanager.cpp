#include "thememanager.h"
#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QColor>
#include <QStyle> 

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

QString ThemeManager::loadStyleSheet(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QString content = QLatin1String(file.readAll());
    file.close();
    return content;
}

void ThemeManager::applyPalette(Theme theme)
{
    QPalette pal;
    if (theme == Dark) {
        pal.setColor(QPalette::Window, QColor(30, 30, 30));
        pal.setColor(QPalette::WindowText, Qt::white);
        pal.setColor(QPalette::Base, QColor(40, 40, 40));
        pal.setColor(QPalette::AlternateBase, QColor(50, 50, 50));
        pal.setColor(QPalette::ToolTipBase, Qt::white);
        pal.setColor(QPalette::ToolTipText, Qt::white);
        pal.setColor(QPalette::Text, Qt::white);
        pal.setColor(QPalette::Button, QColor(50, 50, 50));
        pal.setColor(QPalette::ButtonText, Qt::white);
        pal.setColor(QPalette::BrightText, Qt::red);
        pal.setColor(QPalette::Link, QColor(26, 115, 232));
        pal.setColor(QPalette::Highlight, QColor(26, 115, 232));
        pal.setColor(QPalette::HighlightedText, Qt::black);
    } else {
        pal = QApplication::style()->standardPalette();
    }
    qApp->setPalette(pal);
}

void ThemeManager::applyTheme(Theme theme)
{
    m_currentTheme = theme;
    QString prefix = theme == Dark ? ":/themes/resources/dark.qss" : ":/themes/resources/light.qss";
    QString styleSheet = loadStyleSheet(prefix);
    if (!styleSheet.isEmpty()) {
        qApp->setStyleSheet(styleSheet);
    } else {
        qApp->setStyleSheet(QString());
    }
    applyPalette(theme);
    emit themeChanged(theme);
}
