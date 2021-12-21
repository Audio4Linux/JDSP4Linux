#ifndef SCROLLTEXT_H
#define SCROLLTEXT_H

#include <QStaticText>
#include <QTimer>
#include <QWidget>

class ScrollText :
	public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QString text READ text WRITE setText)
	Q_PROPERTY(QString separator READ separator WRITE setSeparator)

public:
	explicit ScrollText(QWidget *parent = 0);
	QString      text() const;
	void         setText(QString text);

	QString      separator() const;
	void         setSeparator(QString separator);

protected:
	virtual void paintEvent(QPaintEvent*);
	virtual void resizeEvent(QResizeEvent*);

private:
	void         updateText();

	QString _text;
	QString _separator;
	QStaticText staticText;
	int singleTextWidth;
	QSize wholeTextSize;
	int leftMargin;
	bool scrollEnabled;
	int scrollPos;
	QImage alphaChannel;
	QImage buffer;
	QTimer timer;

private slots:
	virtual void timer_timeout();

};

#endif // SCROLLTEXT_H
