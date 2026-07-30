#include "wavewidget.h"
#include "thememanager.h" 
#include <QPainter>

WaveWidget::WaveWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(120);
    setStyleSheet("background-color: transparent;");
}

void WaveWidget::addDataPoint(double value)
{
    dataPoints.append(value);
    if (dataPoints.size() > maxPoints)
        dataPoints.removeFirst();
    update();
}

void WaveWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (dataPoints.size() < 2 || width() <= 0 || height() <= 0)
        return;

    // 从主题管理器获取颜色
    bool isDark = (ThemeManager::instance().currentTheme() == ThemeManager::Dark);
    QColor penColor = isDark ? QColor(100, 200, 255) : QColor(0, 120, 215);

    double stepX = (double)width() / (maxPoints - 1);
    double height = this->height();
    double minY = 40.0, maxY = 120.0;

    QPen pen(penColor, 2);
    painter.setPen(pen);

    QPolygonF polyline;
    for (int i = 0; i < dataPoints.size(); ++i) {
        double x = i * stepX;
        double y = height - ((dataPoints[i] - minY) / (maxY - minY)) * height;
        y = qBound(0.0, y, height);
        polyline << QPointF(x, y);
    }
    painter.drawPolyline(polyline);
}