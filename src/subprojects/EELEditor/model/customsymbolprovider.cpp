#include "customsymbolprovider.h"
#include "utils/templateextension.h"
#include <QDebug>
CustomSymbolProvider::CustomSymbolProvider()
{
}

void CustomSymbolProvider::setLanguageSpecs(EELCompleter *completer, EELHighlighter *highlighter)
{
    mCompleter = completer;
    mHighlighter = highlighter;
}

void CustomSymbolProvider::setCodeEditorModule(CodeEditor *codeEditor)
{
    mCodeEditor = codeEditor;
}

void CustomSymbolProvider::setCodeOutlineModule(CodeOutline *codeOutline)
{
    mCodeOutline = codeOutline;
}

void CustomSymbolProvider::connectSignals(){
    connect(mCodeOutline,&CodeOutline::annotationSelected,[this](AnnotationDefinition def){
        mCodeEditor->goToLine(def.line);
    });
    connect(mCodeOutline,&CodeOutline::functionSelected,[this](FunctionDefinition def){
        mCodeEditor->goToLine(def.line);
    });
    connect(mCodeEditor,&CodeEditor::backendRefreshRequired,this,&CustomSymbolProvider::reloadSymbols);
    connect(mCodeEditor,&CodeEditor::cursorRefreshed,[this]{
        mCodeOutline->goToFunctionByLine(mCodeEditor->getCurrentLine());
    });
}

void CustomSymbolProvider::reloadSymbols(){
    auto newFuncDefs = mCodeEditor->findFunctions();
    auto newAnnoDefs = mCodeEditor->findAnnotations();

    bool needGlobalReload = false;
    if(!compareSymbols(mFuncDefs,newFuncDefs)){
        mHighlighter->loadCustomFunctionRules(newFuncDefs);
        mCompleter->removeCustomItems();
        mCompleter->appendDefinitions(newFuncDefs);
        needGlobalReload = true;
    }

    mCodeOutline->updateFunctions(newFuncDefs);
    mCodeOutline->updateAnnotations(newAnnoDefs);

    if(needGlobalReload)
        mHighlighter->rehighlight();

    mFuncDefs = newFuncDefs;
    mAnnoDefs = newAnnoDefs;
}

template<typename T>
bool CustomSymbolProvider::compareSymbols(T defsA, T defsB){
    if(defsA.size() != defsB.size())
        return false;

    for(auto defA : defsA){
        bool found = false;
        for(auto defB : defsB){
            if(defB == defA)
                found = true;
        }
        if(!found)
            return false;
    }
    return true;
}
