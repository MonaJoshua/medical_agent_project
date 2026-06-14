#include "wavewidget.h"
#include <QPainter>

WaveWidget::WaveWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(150);
    setStyleSheet("background-color: black;");
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

    if (dataPoints.size() < 2) return;
    if (width() <= 0 || height() <= 0) return;

    double width = this->width();
    double height = this->height();
    double stepX = width / (maxPoints - 1);
    double minY = 0.0, maxY = 120.0;

    QPen pen(Qt::green, 2);
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
