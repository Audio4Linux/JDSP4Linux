#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <QStringList>

class StringUtils
{
public:  
    static QStringList tidyStringList(QStringList list);
    static QString findAfterLineNumber(QString input, int line);
    static int countLinesToClosingBracket(QString input, QChar open, QChar close);
};

#endif // STRINGUTILS_H
