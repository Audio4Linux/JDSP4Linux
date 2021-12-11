#ifndef AEQPREVIEWPLOT_H
#define AEQPREVIEWPLOT_H

#include <qcustomplot.h>

class AeqPreviewPlot : public QCustomPlot
{
    Q_OBJECT
public:
    AeqPreviewPlot(QWidget* parent = nullptr);
};

#endif // AEQPREVIEWPLOT_H
