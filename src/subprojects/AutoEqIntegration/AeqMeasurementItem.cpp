#include "AeqMeasurementItem.h"
#include "ui_AeqMeasurementItem.h"

AeqMeasurementItem::AeqMeasurementItem(AeqMeasurement measurement, QWidget *parent) :
	QWidget(parent),
    ui(new Ui::AeqMeasurementItem),
    measurement(measurement)
{
	ui->setupUi(this);

    ui->title->setText(measurement.name);
    ui->desc->setText(measurement.source);
    ui->bestIndicator->setVisible(measurement.isBest);
}

AeqMeasurementItem::~AeqMeasurementItem()
{
	delete ui;
}
