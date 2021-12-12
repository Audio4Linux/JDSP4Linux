#include "AeqPreviewPlot.h"

#include <qtcsv/reader.h>
#include <qtcsv/variantdata.h>
#include <QBuffer>
#include <QApplication>
#include <QAction>
#include <QMenu>

AeqPreviewPlot::AeqPreviewPlot(QWidget* parent) : QCustomPlot(parent)
{
    setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    xAxis->setRange(QCPRange(20,24000));
    xAxis->setLabel("Frequency (Hz)");

    yAxis->setRange(QCPRange(-10,10));
    yAxis->setLabel("Amplitude (dBr)");

    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    xAxis->setTicker(logTicker);
    xAxis->setScaleType(QCPAxis::stLogarithmic);
    rescaleAxes();

    QFont legendFont = font();
    legendFont.setPointSize(10);
    legend->setFont(legendFont);
    legend->setSelectedFont(legendFont);
    legend->setVisible(true);

    axisRect()->insetLayout()->setInsetAlignment(0, (Qt::Alignment)(Qt::AlignBottom|Qt::AlignRight));

    plotLayout()->insertRow(0);
    titleElement = new QCPTextElement(this, "", QFont("sans", 10));
    plotLayout()->addElement(0, 0, titleElement);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QCustomPlot::customContextMenuRequested, this, &AeqPreviewPlot::onContextMenuRequest);
    connect(this, &QCustomPlot::mouseMove, this, &AeqPreviewPlot::onHover);
    connect(this, &QCustomPlot::legendClick, this, &AeqPreviewPlot::onLegendClick);

}

void AeqPreviewPlot::importCsv(const QString &csv, const QString& title)
{
    useGraphicEq = false;

    clearItems();
    clearGraphs();

    titleElement->setText(title);

    auto utf8 = csv.toUtf8();
    auto buffer = QBuffer(&utf8);
    buffer.open(QFile::ReadOnly);

    QtCSV::VariantData variant;
    if(!QtCSV::Reader::readToData(buffer, variant))
    {
        qWarning() << "AeqPreviewPlot::importCsv: failed to parse CSV";
        return;
    }
    buffer.close();

    if(variant.rowCount() <= 1)
    {
        qWarning() << "AeqPreviewPlot::importCsv: empty CSV";
        return;
    }

#define ADD(name,label,color,width) \
    auto name = addGraph(); \
    name->setName(label); \
    QPen name##_pen; \
    name##_pen.setColor(color); \
    name##_pen.setWidthF(width); \
    name->setPen(name##_pen);

    ADD(target, "Target", QColor(173,216,230) /* light blue */,4)
    ADD(smoothed, "Raw (smoothed)", QColor(Qt::lightGray), 4)
    ADD(error_smoothed, "Error (smoothed)", QColor(254, 169, 147) /* light salmon */, 4)
    ADD(equalization, "Equalization", QColor(144,238,144) /* light green */, 4)

    ADD(raw, "Raw", QColor(Qt::black), 1)
    ADD(error, "Error", QColor(Qt::red), 1)
    // ADD(parametric_eq, "Parametric EQ", QColor(0,100,0) /* dark green */, 1)
    // ADD(fixed_band_eq, "Fixed Band EQ", QColor(0,100,0) /* dark green */,1)
    ADD(equalized_raw, "Equalized", QColor(Qt::blue),1)

    /* Add in correct order */
    target->addToLegend(legend);
    smoothed->addToLegend(legend);
    error_smoothed->addToLegend(legend);
    raw->addToLegend(legend);
    error->addToLegend(legend);
    equalization->addToLegend(legend);
    equalized_raw->addToLegend(legend);

#undef ADD

    double minY = -10, maxY = 2;
    for (int row = 1; row < variant.rowCount(); row++)
    {
        auto val = variant.rowValues(row);

        if(val.length() < 11)
        {
            qWarning() << "AeqPreviewPlot::importCsv: CSV columns are missing";
            continue;
        }


        for(int col = 1; col < 11; col++)
        {

#define CASE(column,name) \
    case column: { \
    double y = val[col].toDouble(); \
    if(y > maxY) \
        maxY = y; \
    if(y < minY) \
        minY = y; \
    name->addData(val[0].toDouble(), y); \
    break; \
        }

            switch(col)
            {
                CASE(1,raw)
                CASE(2,error)
                CASE(3,smoothed)
                CASE(4,error_smoothed)
                CASE(5,equalization)
                /* CASE(6,parametric_eq) */
                /* CASE(7,fixed_band_eq) */
                CASE(8,equalized_raw)
                /* column 9: equalized_smoothed unused */
                CASE(10,target)
            }

#undef CASE
        }

        yAxis->setRange(QCPRange(minY - 3, maxY + 3));
        replot(QCustomPlot::rpQueuedReplot);
    }
}

void AeqPreviewPlot::importGraphicEq(const QString &graphic, const QString& title)
{
    useGraphicEq = true;

    clearItems();
    clearGraphs();

    titleElement->setText(title);

    auto g = addGraph(xAxis, yAxis);
    g->setName("Equalization (+preamp)");
    g->addToLegend(legend);

    QPen pen; \
    pen.setColor(QColor(119, 194, 119) /* green */);
    pen.setWidthF(4);
    g->setPen(pen);

    auto dataset = graphic;
    dataset.replace("GraphicEQ: ", "");

    double minY = -10, maxY = 2;
    for(const auto& pair : dataset.split(";"))
    {
        auto set = pair.trimmed().split(" ");
        if(set.count() >= 2)
        {
            double y = set[1].toDouble();
            if(y > maxY)
                maxY = y;
            if(y < minY)
                minY = y;

            g->addData(set[0].toDouble(), y);
        }
    }

    yAxis->setRange(QCPRange(minY - 3, maxY + 3));
    replot(QCustomPlot::rpQueuedReplot);
}

void AeqPreviewPlot::onHover(QMouseEvent *event)
{
    int x = (int) xAxis->pixelToCoord(event->pos().x());
    float y = (float) yAxis->pixelToCoord(event->pos().y());
    if(x < 0 || x > 24000)
    {
        return;
    }

    setToolTip(QString("x=%1Hz; y=%2dBr").arg(x).arg(y));
}


void AeqPreviewPlot::onContextMenuRequest(QPoint pos)
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    if (legend->selectTest(pos, false) >= 0)
    {
        menu->addAction("Move to top left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignLeft));
        menu->addAction("Move to top center", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignHCenter));
        menu->addAction("Move to top right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignRight));
        menu->addAction("Move to bottom right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom|Qt::AlignRight));
        menu->addAction("Move to bottom left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom|Qt::AlignLeft));
    }
    menu->popup(mapToGlobal(pos));
}

void AeqPreviewPlot::moveLegend()
{
    if (QAction* contextAction = qobject_cast<QAction*>(sender()))
    {
        bool ok;
        int dataInt = contextAction->data().toInt(&ok);
        if (ok)
        {
            axisRect()->insetLayout()->setInsetAlignment(0, (Qt::Alignment)dataInt);
            replot();
        }
    }
}

void AeqPreviewPlot::onLegendClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event)
{
    Q_UNUSED(legend)
    Q_UNUSED(event)

    if (item)
    {
        QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
        bool visible = plItem->plottable()->visible();
        plItem->plottable()->setVisible(!visible);
        plItem->setTextColor(!visible ? QColor(Qt::black) : QColor(Qt::gray));
        replot();
    }
}
