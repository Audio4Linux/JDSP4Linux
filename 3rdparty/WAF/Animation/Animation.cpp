/*
 * Copyright (C) 2015  Dimka Novikov, to@dimkanovikov.pro
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * Full license: https://github.com/dimkanovikov/WidgetAnimationFramework/blob/master/LICENSE
 */

#include "Animation.h"
#include "AnimationPrivate.h"

#include "../AbstractAnimator.h"

#include "SideSlide/SideSlideAnimator.h"
#include "Slide/SlideAnimator.h"
#include "CircleFill/CircleFillAnimator.h"
#include "Expand/ExpandAnimator.h"

using WAF::Animation;
using WAF::AnimationPrivate;
using WAF::AbstractAnimator;
using WAF::SideSlideAnimator;
using WAF::SlideAnimator;
using WAF::CircleFillAnimator;
using WAF::ExpandAnimator;


int Animation::sideSlideIn(QWidget* _widget, WAF::ApplicationSide _side, bool _decorateBackground)
{
    const bool IN = true;
    return sideSlide(_widget, _side, _decorateBackground, IN);
}

int Animation::sideSlideOut(QWidget* _widget, WAF::ApplicationSide _side, bool _decorateBackground)
{
    const bool OUT = false;
    return sideSlide(_widget, _side, _decorateBackground, OUT);
}

int Animation::sideSlide(QWidget* _widget, WAF::ApplicationSide _side, bool _decorateBackground, bool _in)
{
    const AnimationPrivate::AnimatorType animatorType = AnimationPrivate::SideSlide;
    AbstractAnimator* animator = 0;
    if (pimpl()->hasAnimator(_widget, animatorType)) {
        animator = pimpl()->animator(_widget, animatorType);
        if (SideSlideAnimator* sideSlideAnimator = qobject_cast<SideSlideAnimator*>(animator)) {
            sideSlideAnimator->setApplicationSide(_side);
            sideSlideAnimator->setDecorateBackground(_decorateBackground);
        }
    } else {
        SideSlideAnimator* sideSlideAnimator = new SideSlideAnimator(_widget);
        sideSlideAnimator->setApplicationSide(_side);
        sideSlideAnimator->setDecorateBackground(_decorateBackground);
        animator = sideSlideAnimator;

        pimpl()->saveAnimator(_widget, animator, animatorType);
    }

    return runAnimation(animator, _in);
}

int Animation::slideIn(QWidget* _widget, WAF::AnimationDirection _direction, bool _fixBackground, bool _fixStartSize)
{
    const bool IN = true;
    return slide(_widget, _direction, _fixBackground, _fixStartSize, IN);
}

int Animation::slideOut(QWidget* _widget, WAF::AnimationDirection _direction, bool _fixBackground, bool _fixStartSize)
{
    const bool OUT = false;
    return slide(_widget, _direction, _fixBackground, _fixStartSize, OUT);
}

int Animation::slide(QWidget* _widget, WAF::AnimationDirection _direction, bool _fixBackground, bool _fixStartSize, bool _in)
{
    const AnimationPrivate::AnimatorType animatorType = AnimationPrivate::Slide;
    AbstractAnimator* animator = 0;
    if (pimpl()->hasAnimator(_widget, animatorType)) {
        animator = pimpl()->animator(_widget, animatorType);
        if (SlideAnimator* slideAnimator = qobject_cast<SlideAnimator*>(animator)) {
            slideAnimator->setAnimationDirection(_direction);
            slideAnimator->setFixBackground(_fixBackground);
            slideAnimator->setFixStartSize(_fixStartSize);
        }
    } else {
        SlideAnimator* slideAnimator = new SlideAnimator(_widget);
        slideAnimator->setAnimationDirection(_direction);
        slideAnimator->setFixBackground(_fixBackground);
        slideAnimator->setFixStartSize(_fixStartSize);
        animator = slideAnimator;

        pimpl()->saveAnimator(_widget, animator, animatorType);
    }

    return runAnimation(animator, _in);
}

int Animation::circleFillIn(QWidget* _widget, const QPoint& _startPoint, const QColor& _fillColor, bool _hideAfterFinish)
{
    const bool IN = true;
    return circleFill(_widget, _startPoint, _fillColor, _hideAfterFinish, IN);
}

int Animation::circleFillOut(QWidget* _widget, const QPoint& _startPoint, const QColor& _fillColor, bool _hideAfterFinish)
{
    const bool OUT = false;
    return circleFill(_widget, _startPoint, _fillColor, _hideAfterFinish, OUT);
}

int Animation::circleFill(QWidget* _widget, const QPoint& _startPoint, const QColor& _fillColor, bool _hideAfterFinish, bool _in)
{
    const AnimationPrivate::AnimatorType animatorType = AnimationPrivate::CircleFill;
    AbstractAnimator* animator = 0;
    if (pimpl()->hasAnimator(_widget, animatorType)) {
        animator = pimpl()->animator(_widget, animatorType);
        if (CircleFillAnimator* circleFillAnimator = qobject_cast<CircleFillAnimator*>(animator)) {
            circleFillAnimator->setStartPoint(_startPoint);
            circleFillAnimator->setFillColor(_fillColor);
            circleFillAnimator->setHideAfterFinish(_hideAfterFinish);
        }
    } else {
        CircleFillAnimator* circleFillAnimator = new CircleFillAnimator(_widget);
        circleFillAnimator->setStartPoint(_startPoint);
        circleFillAnimator->setFillColor(_fillColor);
        circleFillAnimator->setHideAfterFinish(_hideAfterFinish);
        animator = circleFillAnimator;

        pimpl()->saveAnimator(_widget, animator, animatorType);
    }

    return runAnimation(animator, _in);
}

int Animation::expand(QWidget* _widget, const QRect& _expandRect, const QColor& _fillColor, bool _in)
{
    const AnimationPrivate::AnimatorType animatorType = AnimationPrivate::Expand;
    AbstractAnimator* animator = 0;
    if (pimpl()->hasAnimator(_widget, animatorType)) {
        animator = pimpl()->animator(_widget, animatorType);
        if (ExpandAnimator* expandAnimator = qobject_cast<ExpandAnimator*>(animator)) {
            expandAnimator->setExpandRect(_expandRect);
            expandAnimator->setFillColor(_fillColor);
        }
    } else {
        ExpandAnimator* expandAnimator = new ExpandAnimator(_widget);
        expandAnimator->setExpandRect(_expandRect);
        expandAnimator->setFillColor(_fillColor);
        animator = expandAnimator;

        pimpl()->saveAnimator(_widget, animator, animatorType);
    }

    return runAnimation(animator, _in);
}

int Animation::runAnimation(AbstractAnimator* _animator, bool _in)
{
    if (_in) {
        _animator->animateForward();
    } else {
        _animator->animateBackward();
    }

    return _animator->animationDuration();
}

AnimationPrivate* Animation::m_pimpl = 0;
AnimationPrivate* Animation::pimpl()
{
    if (m_pimpl == 0) {
        m_pimpl = new AnimationPrivate;
    }

    return m_pimpl;
}
