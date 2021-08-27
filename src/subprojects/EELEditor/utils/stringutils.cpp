#include "stringutils.h"

#include <QDebug>
#include <QRegularExpression>

QStringList StringUtils::tidyStringList(QStringList list){
    QStringList newList;
    for(auto string : list)
        newList.append(string.trimmed());
    return newList;
}

QString StringUtils::findAfterLineNumber(QString input, int line){
    QString result;
    auto lines = input.split("\n");
    for(int i = line - 1; i < lines.count(); i++)
        result += lines.at(i) + "\n";
    return result;
}
int StringUtils::countLinesToClosingBracket(QString input, QChar open, QChar close){
    QRegularExpression reA(R"(/\*[\s\S]+\*/)");

    QRegularExpressionMatchIterator i = reA.globalMatch(input);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        if (match.hasMatch()) {
             int count = match.captured(0).count(QChar('\n'));

             QString repl;
             for(int i = 0; i < count; i++)
                 repl += "\n";

             input.replace(match.captured(0),repl);
        }
    }

    input.remove(QRegExp(R"(/\*[\s\S]+\*/)"));
    input.remove(QRegExp(R"(//[^\n]*)"));
    int linenum = 0;
    int openedBrackets = 0;
    bool bracketFound = false;
    for(auto character : input){
        if(bracketFound && openedBrackets == 0)
            return linenum;
        if(character == open){
            openedBrackets++;
            bracketFound = true;
        } else if(character == close)
            openedBrackets--;

        if(character == '\n')
            linenum++;
    }
    return -1;
}
