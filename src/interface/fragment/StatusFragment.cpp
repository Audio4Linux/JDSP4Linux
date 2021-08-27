#include "StatusFragment.h"
#include "ui_StatusFragment.h"

#include <QDebug>
#include <QMessageBox>

StatusFragment::StatusFragment(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::StatusDialog)
{
	ui->setupUi(this);
	/*if(!dbus->isValid()){
	    emit closePressed();
	    return;
	   }
	   ui->gst_plugin_ver->setText(dbus->GetVersion());

	   uint versiondata_core = dbus->GetDriverStatus(DBusProxy::PARAM_GET_DRIVER_VERSION);
	   QString versionhex_core = QString("%1").arg(versiondata_core, 0, 16);
	   QString versionfinal_core("");
	   int count = 0;
	   for(auto c:versionhex_core){
	    if((count%2)!=0)versionfinal_core += '.';
	    else versionfinal_core += c;
	    count++;
	   }
	   ui->core_ver->setText(versionfinal_core);

	   int proc = dbus->GetDriverStatus(DBusProxy::PARAM_GET_DRVCANWORK);
	   ui->proc->setText(proc ? tr("Processing") : tr("Not processing"));
	   int enab = dbus->GetDriverStatus(DBusProxy::PARAM_GET_ENABLED);
	   ui->enabled->setText(enab ? tr("Enabled") : tr("Disabled"));

	   int samplerate = dbus->GetDriverStatus(DBusProxy::PARAM_GET_SAMPLINGRATE);
	   ui->samplerate->setText(QString::number(samplerate));*/

	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &StatusFragment::closePressed);

}

StatusFragment::~StatusFragment()
{
	delete ui;
}