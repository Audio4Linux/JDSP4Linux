#ifndef FRAGMENTHOSTPRIVATE_H
#define FRAGMENTHOSTPRIVATE_H

#include "BaseFragment.h"

#include <QFrame>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <WAF/Animation/Animation.h>

class FragmentHostPrivate : public QWidget
{
    Q_OBJECT
public:
    FragmentHostPrivate(BaseFragment* fragment, WAF::ApplicationSide side, QWidget* parent = nullptr) : QWidget(parent)
    {
        _fragment = fragment;
        _side = side;

        _frame = new QFrame(this);
        _frameLayout = new QVBoxLayout(_frame);

        _frameLayout->addWidget(_fragment);
        _frame->setProperty("menu", false);
        _frame->hide();
        _frame->setAutoFillBackground(true);

        connect(fragment, &BaseFragment::closePressed, this, &FragmentHostPrivate::slideOut);
    }

    BaseFragment* fragment() const
    {
        return _fragment;
    }

public slots:
    void slideIn()
    {
        WAF::Animation::sideSlideIn(_frame, _side);
    }

    void slideOut()
    {
        _frame->update();
        _frame->repaint();
        WAF::Animation::sideSlideOut(_frame, _side);
    }

    void slideOutIn()
    {
        WAF::Animation::sideSlideOut(_frame, _side);
        _frame->update();
        _frame->repaint();
        QTimer::singleShot(500, this, [ = ] {
            WAF::Animation::sideSlideIn(_frame, _side);
        });
    }

protected:
    WAF::ApplicationSide _side;
    BaseFragment* _fragment;
    QFrame* _frame;
    QVBoxLayout* _frameLayout;
};

#endif // FRAGMENTHOSTPRIVATE_H
