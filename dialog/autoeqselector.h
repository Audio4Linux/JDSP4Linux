#ifndef AUTOEQSELECTOR_H
#define AUTOEQSELECTOR_H

#include <QDialog>
#include <QThread>

#include <misc/autoeqclient.h>

namespace Ui {
class AutoEQSelector;
}

class AutoEQSelector : public QDialog
{
    Q_OBJECT

public:
    explicit AutoEQSelector(QWidget *parent = nullptr);
    ~AutoEQSelector();
    HeadphoneMeasurement getSelection();
protected:
    void appendToList(QueryResult result);
private slots:
    void updateDetails();
    void doQuery();
private:
    Ui::AutoEQSelector *ui;
    QSize imgSizeCache;
};

#endif // AUTOEQSELECTOR_H
