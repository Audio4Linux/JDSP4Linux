#include "interface/QMessageOverlay.h"
#include "OverlayMsgProxy.h"

#include <QFile>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QTextStream>

OverlayMsgProxy::OverlayMsgProxy(QWidget *_obj)
{
	lightBox = new QMessageOverlay(_obj, false);
	obj      = _obj;
}

void OverlayMsgProxy::openError(QString title,
                                QString desc,
                                QString close)
{
	openBase(title, desc, ":/icons/error.svg", close, "#d72828");
}

void OverlayMsgProxy::openNormal(QString title,
                                 QString desc,
                                 QString color)
{
	openBase(title, desc, "", tr("Close"), color);
}

void OverlayMsgProxy::openBase(QString title,
                               QString desc,
                               QString icon,
                               QString close,
                               QString color)
{
	QLabel      *lbTitle       = new QLabel(title);
	QLabel      *lbIcon        = new QLabel;
	QLabel      *lbDescription = new QLabel(desc);
	QPushButton *lbClose       = new QPushButton(close);
	QGridLayout *lbLayout      = new QGridLayout;

	lbTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: white");
	lbIcon->setPixmap(QPixmap(icon));
	lbDescription->setStyleSheet("color: white");
	lbDescription->setWordWrap(true);
	lbClose->setMinimumHeight(30);

	lbLayout->setRowStretch(0, 1);
	lbLayout->setColumnStretch(0, 1);
	lbLayout->addWidget(lbTitle, 1, 1);
	lbLayout->addWidget(lbTitle, 1, 1);
	lbLayout->addWidget(lbIcon,  1, 2, Qt::AlignRight);
	lbLayout->setColumnStretch(3, 1);
	lbLayout->addWidget(lbDescription, 2, 1, 1, 2);
	lbLayout->addWidget(lbClose,       3, 2);
	lbLayout->setRowStretch(4, 1);

	QFile                   f(":/styles/overlay.qss");
	QTextStream             ts(&f);

	f.open(QFile::ReadOnly | QFile::Text);
	lbClose->setStyleSheet(ts.readAll().replace("#e67e22", color));

	QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect();
	QPropertyAnimation     *a   = new QPropertyAnimation(eff, "opacity");

	lightBox->setGraphicsEffect(eff);
	lightBox->show();
	a->setDuration(500);
	a->setStartValue(0);
	a->setEndValue(1);
	a->setEasingCurve(QEasingCurve::InBack);
	a->start(QPropertyAnimation::DeleteWhenStopped);

	connect(lbClose, &QPushButton::clicked, this,     &OverlayMsgProxy::buttonPressed);
	connect(lbClose, &QPushButton::clicked, lightBox, [lbClose, this]() {
		lbClose->setEnabled(false);
		QGraphicsOpacityEffect *eff2 = new QGraphicsOpacityEffect();
		lightBox->setGraphicsEffect(eff2);
		QPropertyAnimation *a        = new QPropertyAnimation(eff2, "opacity");
		a->setDuration(500);
		a->setStartValue(1);
		a->setEndValue(0);
		a->setEasingCurve(QEasingCurve::OutBack);
		a->start(QPropertyAnimation::DeleteWhenStopped);
		connect(a, &QAbstractAnimation::finished, [this]() {
			lightBox->hide();
		});
	});
	lightBox->setLayout(lbLayout);
}

void OverlayMsgProxy::hide()
{
	lightBox->hide();
}