#ifndef BASEFRAGMENT_H
#define BASEFRAGMENT_H

#include <QWidget>

class BaseFragment : public QWidget {
    Q_OBJECT

public:
    BaseFragment(QWidget* parent = nullptr) : QWidget(parent){};

signals:
    void closePressed();

};

#endif // BASEFRAGMENT_H
