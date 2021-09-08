#include "codeeditor.h"
#include <QTextBlock>
#include <QRegularExpression>
#include <QDebug>
#include <QSyntaxStyle.hpp>
#include <model/codecontainer.h>
#include <utils/stringutils.h>

CodeEditor::CodeEditor(QWidget* parent) :
    QCodeEditor(parent)
{
    refreshTick = new QTimer(this);
    connect(refreshTick, &QTimer::timeout, this, &CodeEditor::fireRefreshSignal);

    connect(this,&QCodeEditor::cursorPositionChanged,[=]{refreshCursorSignalQueued = true;});
    connect(document(),&QTextDocument::contentsChanged,this,[=]{refreshSignalQueued = true;});
}

void CodeEditor::fireRefreshSignal(){
    if(refreshSignalQueued)
        emit backendRefreshRequired();
    if(refreshCursorSignalQueued)
        emit cursorRefreshed();
    refreshSignalQueued = false;
    refreshCursorSignalQueued = false;

    if(cont != nullptr)
        cont->code = toPlainText();
}

void CodeEditor::showEvent(QShowEvent *){
        refreshTick->start(400);
}

void CodeEditor::goToLine(int line){
    QTextCursor text_cursor(document()->findBlockByLineNumber(line - 1));
    text_cursor.select(QTextCursor::LineUnderCursor);
    setTextCursor(text_cursor);
}

int CodeEditor::getCurrentLine(){
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);

    int lines = 1;
    while(cursor.positionInBlock()>0) {
        cursor.movePosition(QTextCursor::Up);
        lines++;
    }
    QTextBlock block = cursor.block().previous();

    while(block.isValid()) {
        lines += block.lineCount();
        block = block.previous();
    }
    return lines;
}

QList<FunctionDefinition> CodeEditor::findFunctions(){
    QRegularExpression basicFunction(R"((?<keyword>(?:procedure )|(?:function ))(?<name>\w+)\s?)");
    QRegularExpression function(R"((?<keyword>(?:procedure )|(?:function ))(?<name>\w+)\s?\((?<param>(?:\w+?\s?)+)?\)(?:\s?)*(?:(?:instance\s*?\(\s*?(?<instance>(?:\w+?\s*?)+)\).*?|local\s*?\(\s*?(?<local>(?:\w+?\s*?)+)\).*?)(?:\s?)*){0,2}(?<below>(?:[\s\S](?!(?:procedure )|(?:function )\w+\s?))+))");
    QList<FunctionDefinition> map;
    int linenum = 1;
    QString previousLines;
    for(auto line : toPlainText().split("\n")){

        auto basicMatchIterator = basicFunction.globalMatch(line);
        if(basicMatchIterator.hasNext()){
            auto matchIterator = function.globalMatch(StringUtils::findAfterLineNumber(toPlainText(),linenum));
            if(matchIterator.hasNext()){
                FunctionDefinition def;
                auto match = matchIterator.next();
                def.name = match.captured("name");

                auto params = match.captured("param").split(" ");
                auto locals = match.captured("local").split(" ");
                auto instances = match.captured("instance").split(" ");
                auto content = match.captured("below");

                int endOfFunc = StringUtils::countLinesToClosingBracket(content,'(',')');

                def.parameters = StringUtils::tidyStringList(params);
                def.localVariables = StringUtils::tidyStringList(locals);
                def.instanceVariables = StringUtils::tidyStringList(instances);
                def.line = linenum;

                if(endOfFunc < 0)
                    def.endOfFunction = -1;
                else
                    def.endOfFunction = linenum + endOfFunc;
                def.isProcedure = match.captured("keyword").contains("procedure");

                def.parameters.removeAll(QString(""));
                def.instanceVariables.removeAll(QString(""));
                def.localVariables.removeAll(QString(""));
                map.push_back(def);

                previousLines.append(line);
            }
        }
        linenum++;
    }
    return map;
}

QList<AnnotationDefinition> CodeEditor::findAnnotations(){
    QRegularExpression function(R"((^|(?<=\n))(\@[^\s\W]*))");
    QList<AnnotationDefinition> map;
    int linenum = 1;
    for(auto line : toPlainText().split("\n")){
        auto matchIterator = function.globalMatch(line);
        if(matchIterator.hasNext()){
            AnnotationDefinition def;
            auto match = matchIterator.next();
            def.name = line;
            def.line = linenum;
            map.push_back(def);
        }
        linenum++;
    }
    return map;
}

int CodeEditor::findAnnotationLine(const QString& name){
    AnnotationDefinition def;
    for (auto annotation : this->findAnnotations())
    {
        if(annotation.name == name)
        {
            return annotation.line;
        }
    }
    return -1;
}

void CodeEditor::loadCode(CodeContainer* code)
{
    cont = code;
    setPlainText(code->code);
}

void CodeEditor::loadStyle(QString path)
{
    QFile fl(path);
    if (!fl.open(QIODevice::ReadOnly))
        return;
    auto style = new QSyntaxStyle(this);
    if (!style->load(fl.readAll()))
    {
        delete style;
        return;
    }

    setSyntaxStyle(style);
}

QString CodeEditor::textUnderCursor() const{
    QString eow = "~!@#$%^&*)+}|:<>?,./;'[]\\-=\"";
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    QString mText = tc.selectedText();

    if(eow.contains(mText.left(1))){
        tc.setPosition(tc.selectionStart()-1);
        tc.select(QTextCursor::WordUnderCursor);
        mText = tc.selectedText();
    }

    return mText.trimmed();
}
