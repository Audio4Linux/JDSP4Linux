include($$PWD/asyncplusplus.pri)
include($$PWD/qtpromise/qtpromise.pri)

greaterThan(QT_MAJOR_VERSION, 5) {
    include($$PWD/qtcsv/qtcsv.pri)
}
lessThan(QT_MAJOR_VERSION, 6) {
    include($$PWD/qtcsv-qt5/qtcsv-qt5.pri)
}

!HEADLESS {
    include($$PWD/qcustomplot/qcustomplot.pri)

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
}
