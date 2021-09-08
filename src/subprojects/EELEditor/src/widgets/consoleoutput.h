#ifndef CONSOLEOUTPUT_H
#define CONSOLEOUTPUT_H

#include <QTextBrowser>

class ConsoleOutput : public QTextBrowser
{
    Q_OBJECT
public:
    ConsoleOutput(QWidget* parent = nullptr);
    void printLine(QString line);
    void print(QString text);

    void printHtmlLine(QString line);
    void printHtml(QString text);

    void printErrorLine(QString line);

    bool getAutoScroll() const;
    void setAutoScroll(bool newAutoScroll);

    void printLowPriorityLine(QString line);
private:
    QMenu *menu;
    bool autoScroll = true;
};

#endif // CONSOLEOUTPUT_H
