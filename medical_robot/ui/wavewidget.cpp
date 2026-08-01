// wavewidget.cpp
// 动态波形绘制组件
#include "wavewidget.h"
#include "thememanager.h"
#include <QPainter>

WaveWidget::WaveWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setStyleSheet("background-color: transparent;");
}

// 添加数据点，超过最大数量时移除最早的点
void WaveWidget::addDataPoint(double value)
{
    dataPoints.append(value);
    if (dataPoints.size() > maxPoints) {
        dataPoints.removeFirst();
    }
    update();
}

// 绘制波形
void WaveWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (dataPoints.size() < 2 || width() <= 0 || height() <= 0) {
        return;
    }

    // 根据主题选择波形颜色
    bool isDark = (ThemeManager::instance().currentTheme() == ThemeManager::Dark);
    QColor penColor = isDark ? QColor(100, 200, 255) : QColor(0, 120, 215);

    // X 轴：等距分布；Y 轴：心率范围 40-120 bpm
    double stepX = static_cast<double>(width()) / (maxPoints - 1);
    double height = static_cast<double>(this->height());
    const double minY = 40.0;
    const double maxY = 120.0;

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