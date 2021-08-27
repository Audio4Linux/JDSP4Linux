#ifndef CUSTOMKEYWORDPROVIDER_H
#define CUSTOMKEYWORDPROVIDER_H
#include "model/eelcompleter.h"
#include "model/eelhighlighter.h"
#include "model/functiondefinition.h"

#include "widgets/codeeditor.h"
#include "widgets/codeoutline.h"

#include <QVector>

class CustomSymbolProvider : public QObject
{
    Q_OBJECT
public:
    CustomSymbolProvider();
    void setLanguageSpecs(EELCompleter* completer, EELHighlighter* highlighter);
    void setCodeEditorModule(CodeEditor* codeEditor);
    void setCodeOutlineModule(CodeOutline* codeOutline);
    template<typename T>
    bool compareSymbols(T defsA, T defsB);
    void connectSignals();
public slots:
    void reloadSymbols();
private:
    CodeEditor* mCodeEditor;
    CodeOutline* mCodeOutline;
    EELCompleter* mCompleter;
    EELHighlighter* mHighlighter;
    QList<FunctionDefinition> mFuncDefs;
    QList<AnnotationDefinition> mAnnoDefs;
};

#endif // CUSTOMKEYWORDPROVIDER_H
