#ifndef WIDGET_H
#define WIDGET_H

#include <QPropertyAnimation>
#include <QSvgWidget>
#include <QParallelAnimationGroup>

class AnimatedJDSPIcon : public QWidget
{
    Q_OBJECT

public:
    AnimatedJDSPIcon(QWidget *parent = nullptr);
    ~AnimatedJDSPIcon();
    void startAnimation();
private:
    QSvgWidget* svgItem = new QSvgWidget(":/icons/parts/jdsp.svg",this);
    QSvgWidget* svgItemU = new QSvgWidget(":/icons/parts/upper.svg",this);
    QSvgWidget* svgItemL = new QSvgWidget(":/icons/parts/lower.svg",this);
    QParallelAnimationGroup *group;
};
#endif // WIDGET_H
