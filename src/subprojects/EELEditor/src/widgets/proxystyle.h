#ifndef PROXYSTYLE_H
#define PROXYSTYLE_H

#include <QMenu>
#include <QProxyStyle>


class ProxyStyle: public QProxyStyle
{
    Q_OBJECT

public:
    ProxyStyle(QStyle * style = 0) : QProxyStyle(style)
    {
    }

    ProxyStyle(const QString & key) : QProxyStyle(key)
    {
    }

    void polish (QWidget * w)
    {
#ifdef __APPLE__
        QMenu* mn = dynamic_cast<QMenu*>(w);
        if(!mn && !w->testAttribute(Qt::WA_MacNormalSize))
            w->setAttribute(Qt::WA_MacSmallSize);
#else
        Q_UNUSED(w);
#endif
    }

    virtual int pixelMetric(QStyle::PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0 ) const
    {
        switch ( metric )
        {
        case QStyle::PM_SmallIconSize:
            return 16;
        default:
            return QProxyStyle::pixelMetric( metric, option, widget );
        }
    }
};

#endif // PROXYSTYLE_H
