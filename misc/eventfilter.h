#ifndef EVENTFILTER_H
#define EVENTFILTER_H
#include <QObject>
#include <QEvent>
class KeyboardFilter : public QObject
{
    Q_OBJECT
public:
    bool eventFilter(QObject *object, QEvent *event)
    {
        Q_UNUSED(object)
        return event->type() == QEvent::KeyPress;
    }
};

#endif // EVENTFILTER_H
