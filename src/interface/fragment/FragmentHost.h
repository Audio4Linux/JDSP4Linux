#ifndef FRAGMENTHOST_H
#define FRAGMENTHOST_H

#include "FragmentHostPrivate.h"

#include <QWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QVariant>
#include <QTimer>

#include <WAF/Animation/Animation.h>

template<class T>
class FragmentHost : public FragmentHostPrivate
{
public:
    FragmentHost(T fragment, WAF::ApplicationSide side, QWidget* parent = nullptr)
        : FragmentHostPrivate(fragment, side, parent) {}

    T fragment() const
    {
        return static_cast<T>(_fragment);
    }
};

#endif // FRAGMENTHOST_H
