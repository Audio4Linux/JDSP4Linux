#ifndef EELHIGHLIGHTER_H
#define EELHIGHLIGHTER_H

#pragma once

// QCodeEditor
#include <QStyleSyntaxHighlighter.hpp> // Required for inheritance
#include <QHighlightRule.hpp>

// Qt
#include <QRegularExpression>
#include <QVector>

#include "model/functiondefinition.h"

class QSyntaxStyle;

/**
 * @brief Class, that describes EEL code
 * highlighter.
 */
class EELHighlighter : public QStyleSyntaxHighlighter
{
    Q_OBJECT
public:

    /**
     * @brief Constructor.
     * @param document Pointer to document.
     */
    explicit EELHighlighter(QTextDocument* document=nullptr);
    void loadCustomFunctionRules(QList<FunctionDefinition> funcDefs, bool append = false);
protected:
    void highlightBlock(const QString& text) override;

private:

    QVector<QHighlightRule> m_highlightRules;
    QList<QHighlightRule> m_customFunctionRules;

    QRegularExpression m_functionPattern;
    QRegularExpression m_defTypePattern;

    QRegularExpression m_commentStartPattern;
    QRegularExpression m_commentEndPattern;
};



#endif // EELHIGHLIGHTER_H
