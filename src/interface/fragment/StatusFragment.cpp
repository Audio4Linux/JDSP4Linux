#include "StatusFragment.h"
#include "ui_StatusFragment.h"

#define STR_(x) #x
#define STR(x) STR_(x)

StatusFragment::StatusFragment(QWidget *parent) :
    BaseFragment(parent),
	ui(new Ui::StatusDialog)
{
	ui->setupUi(this);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &StatusFragment::closePressed);
}

void StatusFragment::updateStatus(const DspStatus &status)
{
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
}

StatusFragment::~StatusFragment()
{
	delete ui;
}
