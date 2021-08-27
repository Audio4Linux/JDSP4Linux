SOURCES += \
    $$PWD/src/QCodeEditor.cpp \
    $$PWD/src/QFramedTextAttribute.cpp \
    $$PWD/src/QLanguage.cpp \
    $$PWD/src/QLineNumberArea.cpp \
    $$PWD/src/QStyleSyntaxHighlighter.cpp \
    $$PWD/src/QSyntaxStyle.cpp

HEADERS += \
    $$PWD/include/QCodeEditor.hpp \
    $$PWD/include/QFramedTextAttribute.hpp \
    $$PWD/include/QHighlightBlockRule.hpp \
    $$PWD/include/QHighlightRule.hpp \
    $$PWD/include/QLanguage.hpp \
    $$PWD/include/QLineNumberArea.hpp \
    $$PWD/include/QStyleSyntaxHighlighter.hpp \
    $$PWD/include/QSyntaxStyle.hpp

INCLUDEPATH += $$PWD/include

RESOURCES += \
    $$PWD/resources/qcodeeditor_resources.qrc
