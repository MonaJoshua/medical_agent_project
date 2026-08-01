#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>

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
    ~ThemeManager() = default;

    // 禁止拷贝（单例）
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    void applyPalette(Theme theme);
    QString loadStyleSheet(const QString& path);

    Theme m_currentTheme = Light;
};

#endif // THEMEMANAGER_H