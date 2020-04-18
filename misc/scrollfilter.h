#ifndef SCROLLFILTER_H
#define SCROLLFILTER_H

#include <QCloseEvent>
#include <QEvent>
#include <QObject>

class ScrollFilter : public QObject{
    Q_OBJECT
public:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        return (event->type() == QEvent::Scroll || event->type() == QEvent::Wheel);
    }
};

#endif // SCROLLFILTER_H
