#ifndef CONFIGITEM_H
#define CONFIGITEM_H

#include "AeqStructs.h"

#include <QWidget>

namespace Ui
{
    class AeqMeasurementItem;
}

class AeqMeasurementItem :
	public QWidget
{
	Q_OBJECT

public:
    explicit AeqMeasurementItem(AeqMeasurement measurement, QWidget *parent = nullptr);
    ~AeqMeasurementItem();

private:
    Ui::AeqMeasurementItem *ui;
    AeqMeasurement measurement;
};

#endif // CONFIGITEM_H
