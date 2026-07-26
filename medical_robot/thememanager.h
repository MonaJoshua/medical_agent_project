#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QPalette>

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    enum Theme { Light, Dark };

    static ThemeManager& instance();

    void applyTheme(Theme theme);
    Theme currentTheme() const { return m_currentTheme; }

signals:
    void themeChanged(Theme theme);

private:
    ThemeManager() = default;
    Theme m_currentTheme = Light;
    QString loadStyleSheet(const QString& path);
    void applyPalette(Theme theme);
};

#endif // THEMEMANAGER_H
