#include "StatusFragment.h"
#include "ui_StatusFragment.h"

#define STR_(x) #x
#define STR(x) STR_(x)

StatusFragment::StatusFragment(DspStatus status, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::StatusDialog)
{
	ui->setupUi(this);

#ifdef USE_PULSEAUDIO
    QString flavor = " (Pulseaudio flavor)";
#else
    QString flavor = " (Pipewire flavor)";
#endif
    ui->app_ver->setText(STR(APP_VERSION) + flavor);
    ui->core_ver->setText(STR(JDSP_VERSION));
    ui->proc->setText(status.IsProcessing ? tr("Processing") : tr("Not processing"));
    ui->format->setText(QString::fromStdString(status.AudioFormat));
    ui->samplerate->setText(QString::fromStdString(status.SamplingRate) + "Hz");

	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &StatusFragment::closePressed);
}

StatusFragment::~StatusFragment()
{
	delete ui;
}
