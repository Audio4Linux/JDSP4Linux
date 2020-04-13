#include "ExpandDecorator.h"

#include <QPainter>

using WAF::ExpandDecorator;


ExpandDecorator::ExpandDecorator(QWidget* _parent) :
	QWidget(_parent)
{

}

QRect ExpandDecorator::expandRect() const
{
	return m_expandRect;
}

void ExpandDecorator::setExpandRect(QRect _expandRect)
{
	if (m_expandRect != _expandRect) {
		m_expandRect = _expandRect;

		update();
	}
}

void ExpandDecorator::grabExpandRect()
{
	m_expandRectPixmap = QPixmap(m_expandRect.size());
	parentWidget()->render(&m_expandRectPixmap, QPoint(), QRegion(m_expandRect));
}

void ExpandDecorator::setFillColor(const QColor& _fillColor)
{
	if (m_fillColor != _fillColor) {
		m_fillColor = _fillColor;
	}
}

void ExpandDecorator::paintEvent(QPaintEvent* _event)
{
	QPainter painter(this);
	painter.setOpacity(qMin((qreal)m_expandRect.height() / height(), 0.4));
	painter.fillRect(rect(), Qt::black);
	painter.setOpacity(1.);
	painter.fillRect(m_expandRect, m_fillColor);
	painter.drawPixmap(m_expandRect.topLeft(), m_expandRectPixmap);
	//
	// Рисуем декорации для создания эффекта наложения сверху
	//
	painter.setPen(Qt::black);
	QPoint left, right;
	left = m_expandRect.topLeft();
	right = m_expandRect.topRight();
	left.setY(left.y() - 1);
	right.setY(right.y() - 1);
	painter.setOpacity(0.1);
	painter.drawLine(left, right);
	//
	left.setY(left.y() - 1);
	right.setY(right.y() - 1);
	painter.setOpacity(0.4);
	painter.drawLine(left, right);
	//
	left.setY(left.y() - 1);
	right.setY(right.y() - 1);
	painter.setOpacity(0.2);
	painter.drawLine(left, right);
	//
	left = m_expandRect.bottomLeft();
	right = m_expandRect.bottomRight();
	left.setY(left.y() + 1);
	right.setY(right.y() + 1);
	painter.setOpacity(0.1);
	painter.drawLine(left, right);
	//
	left.setY(left.y() + 1);
	right.setY(right.y() + 1);
	painter.setOpacity(0.4);
	painter.drawLine(left, right);
	//
	left.setY(left.y() + 1);
	right.setY(right.y() + 1);
	painter.setOpacity(0.2);
	painter.drawLine(left, right);
	//
	left.setY(left.y() + 1);
	right.setY(right.y() + 1);
	painter.setOpacity(0.1);
	painter.drawLine(left, right);

	QWidget::paintEvent(_event);
}
