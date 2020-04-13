#include "qmessageoverlay.h"

#include <QPixmap>
#include <QEvent>
#include <QPaintEvent>
#include <QChildEvent>
#include <QPainter>

QMessageOverlay::QMessageOverlay(QWidget* _parent, bool _folowToHeadWidget) :
	QWidget(_parent),
	m_isInUpdateSelf(false)
{
	Q_ASSERT_X(_parent, "", Q_FUNC_INFO);

	if (_folowToHeadWidget) {
		while (_parent->parentWidget() != nullptr) {
			_parent = _parent->parentWidget();
		}
		setParent(_parent);
	}

	_parent->installEventFilter(this);
	setVisible(false);
}

bool QMessageOverlay::eventFilter(QObject* _object, QEvent* _event)
{
	if (_event->type() == QEvent::ChildAdded) {
		QChildEvent* childEvent = dynamic_cast<QChildEvent*>(_event);
        if (childEvent->child() != this && _object != qobject_cast<QObject*>(parentWidget())) {
			QWidget* parent = parentWidget();
			setParent(nullptr);
			setParent(parent);
		}
	}

	if (isVisible()
		&& _event->type() == QEvent::Resize) {
		updateSelf();
	}
	return QWidget::eventFilter(_object, _event);
}

void QMessageOverlay::paintEvent(QPaintEvent* _event)
{
	QPainter p;
	p.begin(this);
    p.drawPixmap(-1, -1, width(), height(), m_parentWidgetPixmap);
	p.setBrush(QBrush(QColor(0, 0, 0, 220)));
    p.drawRect(-1, -1, width()+1, height()+1);
	p.end();
	QWidget::paintEvent(_event);
}

void QMessageOverlay::showEvent(QShowEvent* _event)
{
	updateSelf();
	QWidget::showEvent(_event);
}

void QMessageOverlay::updateSelf()
{
	if (!m_isInUpdateSelf) {
		m_isInUpdateSelf = true;
		{
			hide();
			resize(parentWidget()->size());
			m_parentWidgetPixmap = grabParentWidgetPixmap();
			show();
		}
		m_isInUpdateSelf = false;
	}
}

QPixmap QMessageOverlay::grabParentWidgetPixmap() const
{
	QPixmap parentWidgetPixmap;
	parentWidgetPixmap = parentWidget()->grab();
	return parentWidgetPixmap;
}
