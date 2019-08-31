#ifndef LOG_H
#define LOG_H

#include <QDialog>

namespace Ui {
class log;
}

class log : public QDialog
{
    Q_OBJECT

public:
    explicit log(QWidget *parent = nullptr);
    ~log();
private slots:
    void reject();
    void updateLog();
private:
    Ui::log *ui;
};

#endif // LOG_H
