// QCodeEditor
#include <QSyntaxStyle.hpp>
#include <QLanguage.hpp>

// Qt
#include <QFile>

#include "eelhighlighter.h"

EELHighlighter::EELHighlighter(QTextDocument* document) :
    QStyleSyntaxHighlighter(document),
    m_highlightRules     (),
    m_functionPattern    (QRegularExpression(R"((((?:procedure )|(?:function ))(\w+)))")),
    m_defTypePattern     (QRegularExpression(R"(^((?:[A-Za-z0-9])+:)([\s\S]\N+))")),
    m_commentStartPattern(QRegularExpression(R"(/\*)")),
    m_commentEndPattern  (QRegularExpression(R"(\*/)"))
{
    Q_INIT_RESOURCE(editorresources);
    QFile fl(":/definitions/eelang.xml");

    if (!fl.open(QIODevice::ReadOnly))
    {
        return;
    }

    QLanguage language(&fl);

    if (!language.isLoaded())
    {
        return;
    }

    auto keys = language.keys();
    for (auto&& key : keys)
    {
        auto names = language.names(key);
        for (auto&& name : names)
        {
            m_highlightRules.append({
                QRegularExpression(QString(R"(\b%1\b)").arg(name)),
                key
            });
        }
    }

    // Numbers
    m_highlightRules.append({
        QRegularExpression(R"(\b((0(x|X)[0-9a-fA-F]*)|(([0-9]+\.?[0-9]*)|(\.[0-9]+))((e|E)(\+|-)?[0-9]+)?)(n|f)?\b)"),
        "Number"
    });

    // Annotations
    m_highlightRules.append({
        QRegularExpression(R"((^|(?<=\n))(\@[^\s\W]*))"),
        "VirtualMethod"
    });

    // Strings
    m_highlightRules.append({
        QRegularExpression(R"("[^\n"]*")"),
        "String"
    });

    // Char
    m_highlightRules.append({
        QRegularExpression(R"('[^\n']*')"),
        "Char"
    });

    // Operator
    m_highlightRules.append({
        QRegularExpression(R"(([-*=+!;:%|^#$]))"),
        "Operator"
    });

    // Brackets
    m_highlightRules.append({
        QRegularExpression(R"(([\{\}\(\)\[\]]))"),
        "Operator"
    });

    //Math literal
    m_highlightRules.append({
        QRegularExpression(R"(\$\w+)"),
        "Char"
    });

    // Escaped Char
    m_highlightRules.append({
        QRegularExpression(R"(\\(\\|[abefnprtv'"?]|[0-3]\d{,2}|[4-7]\d?|x[a-fA-F0-9]{,2}|u[a-fA-F0-9]{,4}|U[a-fA-F0-9]{,8}))"),
        "Preprocessor"
    });

    // Single line
    m_highlightRules.append({
        QRegularExpression(R"(//[^\n]*)"),
        "Comment"
    });
}

void EELHighlighter::loadCustomFunctionRules(QList<FunctionDefinition> funcDefs, bool append){
    if(!append)
        m_customFunctionRules.clear();
    for(FunctionDefinition def : funcDefs){
        QString type = "Function";
        if(def.isProcedure)
            type = "Procedure";
        m_customFunctionRules.append({
            QRegularExpression(QString(R"((?<!(?:function )|(?:procedure ))\b%1\b)").arg(def.name)),
            type
        });
    }
}

void EELHighlighter::highlightBlock(const QString& text)
{
    {
        auto matchIterator = m_defTypePattern.globalMatch(text);

        while (matchIterator.hasNext())
        {
            auto match = matchIterator.next();

            setFormat(
                match.capturedStart(1),
                match.capturedLength(1),
                syntaxStyle()->getFormat("Type")
            );

            setFormat(
                match.capturedStart(2),
                match.capturedLength(2),
                syntaxStyle()->getFormat("Field")
            );
        }
    }

    for (auto& rule : m_customFunctionRules)
    {
        auto matchIterator = rule.pattern.globalMatch(text);

        while (matchIterator.hasNext())
        {
            auto match = matchIterator.next();

            setFormat(
                match.capturedStart(),
                match.capturedLength(),
                syntaxStyle()->getFormat(rule.formatName)
            );
        }
    }

    // Checking for function
    {
        auto matchIterator = m_functionPattern.globalMatch(text);

        while (matchIterator.hasNext())
        {
            auto match = matchIterator.next();

            setFormat(
                match.capturedStart(),
                match.capturedLength(),
                syntaxStyle()->getFormat("Function")
            );
        }
    }


    for (auto& rule : m_highlightRules)
    {
        auto matchIterator = rule.pattern.globalMatch(text);

        while (matchIterator.hasNext())
        {
            auto match = matchIterator.next();

            setFormat(
                match.capturedStart(),
                match.capturedLength(),
                syntaxStyle()->getFormat(rule.formatName)
            );
        }
    }

    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
    {
        startIndex = text.indexOf(m_commentStartPattern);
    }

    while (startIndex >= 0)
    {
        auto match = m_commentEndPattern.match(text, startIndex);

        int endIndex = match.capturedStart();
        int commentLength = 0;

        if (endIndex == -1)
        {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        }
        else
        {
            commentLength = endIndex - startIndex + match.capturedLength();
        }

        setFormat(
            startIndex,
            commentLength,
            syntaxStyle()->getFormat("Comment")
        );
        startIndex = text.indexOf(m_commentStartPattern, startIndex + commentLength);
    }

    // We expect one text block per line
    if(currentBlock().firstLineNumber() == errorLine)
    {
        setFormat(
            0,
            currentBlock().length(),
            syntaxStyle()->getFormat("Error")
        );
    }
}

int EELHighlighter::getErrorLine() const
{
    return errorLine;
}

void EELHighlighter::setErrorLine(int newErrorLine)
{
    errorLine = newErrorLine;
}
