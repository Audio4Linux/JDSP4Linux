#ifndef AEQPREVIEWPLOT_H
#define AEQPREVIEWPLOT_H

#include <qcustomplot.h>

class AeqPreviewPlot : public QCustomPlot
{
    Q_OBJECT
public:
    AeqPreviewPlot(QWidget* parent = nullptr);

    void importCsv(const QString& csv);
    void importGraphicEq(const QString& graphic);

};

#endif // AEQPREVIEWPLOT_H
