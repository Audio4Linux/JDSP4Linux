include($$PWD/asyncplusplus.pri)

SOURCES += \
    $$PWD/WAF/Animation/Animation.cpp \
    $$PWD/WAF/Animation/CircleFill/CircleFillAnimator.cpp \
    $$PWD/WAF/Animation/CircleFill/CircleFillDecorator.cpp \
    $$PWD/WAF/Animation/Expand/ExpandAnimator.cpp \
    $$PWD/WAF/Animation/Expand/ExpandDecorator.cpp \
    $$PWD/WAF/Animation/SideSlide/SideSlideAnimator.cpp \
    $$PWD/WAF/Animation/SideSlide/SideSlideDecorator.cpp \
    $$PWD/WAF/Animation/Slide/SlideAnimator.cpp \
    $$PWD/WAF/Animation/Slide/SlideForegroundDecorator.cpp \
    $$PWD/WebLoader/src/HttpMultiPart_p.cpp \
    $$PWD/WebLoader/src/NetworkQueue_p.cpp \
    $$PWD/WebLoader/src/NetworkRequest.cpp \
    $$PWD/WebLoader/src/WebLoader_p.cpp \
    $$PWD/WebLoader/src/WebRequest_p.cpp \
    $$PWD/WebLoader/src/customcookiejar.cpp \

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
    $$PWD/WAF/WAF.h \
    $$PWD/WebLoader/src/HttpMultiPart_p.h \
    $$PWD/WebLoader/src/NetworkQueue_p.h \
    $$PWD/WebLoader/src/NetworkRequest.h \
    $$PWD/WebLoader/src/NetworkRequestLoader.h \
    $$PWD/WebLoader/src/NetworkRequestPrivate_p.h \
    $$PWD/WebLoader/src/WebLoader_p.h \
    $$PWD/WebLoader/src/WebRequest_p.h \
    $$PWD/WebLoader/src/customcookiejar.h \

INCLUDEPATH += $$PWD/WAF \
               $$PWD
