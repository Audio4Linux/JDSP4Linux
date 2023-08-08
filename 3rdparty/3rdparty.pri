include($$PWD/asyncplusplus.pri)
include($$PWD/qtpromise/qtpromise.pri)
include($$PWD/qcustomplot/qcustomplot.pri)
include($$PWD/qtcsv/qtcsv.pri)

SOURCES += \
    $$PWD/WAF/Animation/Animation.cpp \
    $$PWD/WAF/Animation/CircleFill/CircleFillAnimator.cpp \
    $$PWD/WAF/Animation/CircleFill/CircleFillDecorator.cpp \
    $$PWD/WAF/Animation/Expand/ExpandAnimator.cpp \
    $$PWD/WAF/Animation/Expand/ExpandDecorator.cpp \
    $$PWD/WAF/Animation/SideSlide/SideSlideAnimator.cpp \
    $$PWD/WAF/Animation/SideSlide/SideSlideDecorator.cpp \
    $$PWD/WAF/Animation/Slide/SlideAnimator.cpp \
    $$PWD/WAF/Animation/Slide/SlideForegroundDecorator.cpp

HEADERS += \
    $$PWD/WAF/AbstractAnimator.h \
    $$PWD/WAF/Animation/Animation.h \
    $$PWD/WAF/Animation/AnimationPrivate.h \
    $$PWD/WAF/Animation/CircleFill/CircleFillAnimator.h \
    $$PWD/WAF/Animation/CircleFill/CircleFillDecorator.h \
    $$PWD/WAF/Animation/Expand/ExpandAnimator.h \
    $$PWD/WAF/Animation/Expand/ExpandDecorator.h \
    $$PWD/WAF/Animation/SideSlide/SideSlideAnimator.h \
    $$PWD/WAF/Animation/SideSlide/SideSlideDecorator.h \
    $$PWD/WAF/Animation/Slide/SlideAnimator.h \
    $$PWD/WAF/Animation/Slide/SlideForegroundDecorator.h \
    $$PWD/WAF/WAF.h

INCLUDEPATH += $$PWD/WAF \
               $$PWD
