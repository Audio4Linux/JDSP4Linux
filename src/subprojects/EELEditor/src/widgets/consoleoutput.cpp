#include "consoleoutput.h"

#include <QMenu>
#include <QScrollBar>
#include <QApplication>
#include <QClipboard>

ConsoleOutput::ConsoleOutput(QWidget* parent) : QTextBrowser(parent)
{
    this->setReadOnly(true);
    this->setStyleSheet("QTextEdit { background: black; color: #dddddd; }");
    QFont font;
    font.setFamily("Consolas");
    font.setPointSize(10);
    this->setFont(font);
    this->setContextMenuPolicy(Qt::CustomContextMenu);


    menu = new QMenu(this);

    auto autoScrollAction = new QAction("Auto-scroll", this);
    autoScrollAction->setCheckable(true);
    autoScrollAction->setChecked(true);
    connect(autoScrollAction, &QAction::toggled, this, &ConsoleOutput::setAutoScroll);
    menu->addAction(autoScrollAction);
    auto copyAction = new QAction("Copy selection", this);
    connect(copyAction, &QAction::triggered, this, &ConsoleOutput::copy);
    menu->addAction(copyAction);
    auto copyAllAction = new QAction("Copy all", this);
    connect(copyAllAction, &QAction::triggered, this, [this]{
        qApp->clipboard()->setText(this->toPlainText(), QClipboard::Clipboard);
    });
    menu->addAction(copyAllAction);
    auto clearAction = new QAction("Clear", this);
    connect(clearAction, &QAction::triggered, this, &ConsoleOutput::clear);
    menu->addAction(clearAction);


    connect(this, &ConsoleOutput::customContextMenuRequested, [this](QPoint pos){
        menu->popup(this->mapToGlobal(pos));
    });
}

void ConsoleOutput::printLine(QString line)
{
    print(line + "\n");
}

void ConsoleOutput::print(QString text)
{
    auto textCursor = this->textCursor();
    textCursor.movePosition(QTextCursor::End);
    this->setTextCursor(textCursor);
    this->insertPlainText(QString(text));

    if(autoScroll)
    {
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->maximum());
    }
}

void ConsoleOutput::printHtmlLine(QString line)
{
    printHtml(line + "<br>");
}

void ConsoleOutput::printHtml(QString text)
{
    auto textCursor = this->textCursor();
    textCursor.movePosition(QTextCursor::End);
    this->setTextCursor(textCursor);
    this->insertHtml(QString(text));

    if(autoScroll)
    {
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->maximum());
    }
}

void ConsoleOutput::printErrorLine(QString line)
{
    printHtmlLine("<span style=\"font-weight: 700;color: #FF5050\">" + line + "</span>");
}

void ConsoleOutput::printLowPriorityLine(QString line)
{
    printHtmlLine("<span style=\"font-weight: 700;color: gray\">" + line + "</span>");
}

bool ConsoleOutput::getAutoScroll() const
{
    return autoScroll;
}

void ConsoleOutput::setAutoScroll(bool newAutoScroll)
{
    autoScroll = newAutoScroll;
}

