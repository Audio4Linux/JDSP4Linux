#include "CircleFillDecorator.h"

#include <QPainter>

using WAF::CircleFillDecorator;



CircleFillDecorator::CircleFillDecorator(QWidget* _parent) :
    QWidget(_parent),
    m_radius(0),
    m_fillColor(Qt::white)
{

}

void CircleFillDecorator::setStartPoint(const QPoint& _point)
{
    QPoint localStartPoint = mapFromGlobal(_point);
    if (m_startPoint != localStartPoint) {
        m_startPoint = localStartPoint;
    }
}

int CircleFillDecorator::radius() const
{
    return m_radius;
}

void CircleFillDecorator::setRadius(int _radius)
{
    if (m_radius != _radius) {
        m_radius = _radius;

        update();
    }
}

void CircleFillDecorator::setFillColor(const QColor& _fillColor)
{
    if (m_fillColor != _fillColor) {
        m_fillColor = _fillColor;
    }
}

void CircleFillDecorator::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setPen(m_fillColor);
    painter.setBrush(m_fillColor);
    const qreal opacity = (qreal)m_radius / ((qreal)(width() + height()) / 4.);
    painter.setOpacity(opacity);
    painter.drawEllipse(m_startPoint, m_radius, m_radius);

    QWidget::paintEvent(_event);
}
