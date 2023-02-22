#ifndef FRAGMENTHOSTPRIVATE_H
#define FRAGMENTHOSTPRIVATE_H

#include "BaseFragment.h"

#include <QFrame>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <WAF/Animation/Animation.h>

class FragmentHostPrivate : public QFrame
{
    Q_OBJECT
public:
    FragmentHostPrivate(BaseFragment* fragment, WAF::ApplicationSide side, QWidget* parent = nullptr) : QFrame(parent)
    {
        _fragment = fragment;
        _side = side;

        _frameLayout = new QVBoxLayout(this);
        _frameLayout->addWidget(_fragment);

        this->setProperty("menu", false);
        this->hide();
        this->setAutoFillBackground(true);

        connect(fragment, &BaseFragment::closePressed, this, &FragmentHostPrivate::slideOut);
    }

    BaseFragment* fragment() const
    {
        return _fragment;
    }

public slots:
    void slideIn()
    {
        WAF::Animation::sideSlideIn(this, _side);
    }

    void slideOut()
    {
        this->update();
        this->repaint();
        WAF::Animation::sideSlideOut(this, _side);
    }

    void slideOutIn()
    {
        WAF::Animation::sideSlideOut(this, _side);
        this->update();
        this->repaint();
        QTimer::singleShot(500, this, [this] {
            WAF::Animation::sideSlideIn(this, _side);
        });
    }

protected:
    WAF::ApplicationSide _side;
    BaseFragment* _fragment;
    QVBoxLayout* _frameLayout;
};

#endif // FRAGMENTHOSTPRIVATE_H
