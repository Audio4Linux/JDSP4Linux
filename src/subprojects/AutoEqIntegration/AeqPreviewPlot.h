#ifndef AEQPREVIEWPLOT_H
#define AEQPREVIEWPLOT_H

#include <qcustomplot.h>

class AeqPreviewPlot : public QCustomPlot
{
    Q_OBJECT
public:
    AeqPreviewPlot(QWidget* parent = nullptr);

    void importCsv(const QString& csv, const QString &title);
    void importGraphicEq(const QString &graphic, const QString &title);
private slots:
    void onHover(QMouseEvent* event);

    void onContextMenuRequest(QPoint pos);
    void moveLegend();
    void onLegendClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event);
    void onLegendDoubleClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event);

private:
    void updateBaseColors(bool dark);

    QCPTextElement* titleElement;

};

#endif // AEQPREVIEWPLOT_H
