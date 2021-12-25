#include "StatusFragment.h"
#include "ui_StatusFragment.h"

#include "utils/VersionMacros.h"

StatusFragment::StatusFragment(QWidget *parent) :
    BaseFragment(parent),
	ui(new Ui::StatusDialog)
{
	ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &StatusFragment::closePressed);
}

void StatusFragment::updateStatus(const DspStatus &status)
{
    ui->app_ver->setText(APP_VERSION_FULL);
    ui->core_ver->setText(STR(JDSP_VERSION));
    ui->proc->setText(status.IsProcessing ? tr("Processing") : tr("Not processing"));
    ui->format->setText(QString::fromStdString(status.AudioFormat));
    ui->samplerate->setText(QString::fromStdString(status.SamplingRate) + "Hz");
}

StatusFragment::~StatusFragment()
{
	delete ui;
}
