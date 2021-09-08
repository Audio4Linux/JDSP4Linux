include(../3rdparty/QCodeEditor/QCodeEditor.pri)

SOURCES += \
    $$PWD/eeleditor.cpp \
    $$PWD/utils/stringutils.cpp \
    $$PWD/model/completerdelegate.cpp \
    $$PWD/model/customsymbolprovider.cpp \
    $$PWD/model/eelcompleter.cpp \
    $$PWD/model/eelhighlighter.cpp \
    $$PWD/widgets/codeeditor.cpp \
    $$PWD/widgets/codeoutline.cpp \
    $$PWD/widgets/consoleoutput.cpp \
    $$PWD/widgets/findreplaceform.cpp \
    $$PWD/widgets/projectview.cpp

HEADERS += \
    $$PWD/eeleditor.h \
    $$PWD/utils/stringutils.h \
    $$PWD/model/annotationdefinition.h \
    $$PWD/model/codecontainer.h \
    $$PWD/model/completerdelegate.h \
    $$PWD/model/customsymbolprovider.h \
    $$PWD/model/eelcompleter.h \
    $$PWD/model/eelhighlighter.h \
    $$PWD/model/functiondefinition.h \
    $$PWD/utils/templateextension.h \
    $$PWD/widgets/codeeditor.h \
    $$PWD/widgets/codeoutline.h \
    $$PWD/widgets/consoleoutput.h \
    $$PWD/widgets/findreplaceform.h \
    $$PWD/widgets/projectview.h \
    $$PWD/widgets/proxystyle.h

FORMS += \
    $$PWD/eeleditor.ui \
    $$PWD/widgets/findreplaceform.ui

INCLUDEPATH += $$PWD

RESOURCES += \
    $$PWD/editorresources.qrc
