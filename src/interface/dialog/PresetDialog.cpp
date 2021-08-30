#include "PresetDialog.h"
#include "ui_PresetDialog.h"

#include "config/AppConfig.h"
#include "MainWindow.h"
#include "utils/Common.h"
#include "utils/Log.h"

#include <Animation/Animation.h>

#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMessageBox>
#include <QNetworkReply>

PresetDialog::PresetDialog(QWidget *parent) :
	QDialog(parent),
    ui(new Ui::PresetDialog)
{
	ui->setupUi(this);

	UpdateList();
	connect(ui->add,        SIGNAL(clicked()),                                              SLOT(add()));
	connect(ui->load,       SIGNAL(clicked()),                                              SLOT(load()));
	connect(ui->remove,     SIGNAL(clicked()),                                              SLOT(remove()));
	connect(ui->presetName, SIGNAL(textChanged(QString)),                                   this, SLOT(nameChanged(QString)));
	connect(ui->files,      SIGNAL(customContextMenuRequested(QPoint)),                     this, SLOT(showContextMenu(QPoint)));
	connect(ui->files,      SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this, SLOT(presetIndexChanged()));

	QMenu *menu = new QMenu();
	menu->addAction(tr("Linux Configuration"), this, SLOT(importLinux()));
	ui->importBtn->setMenu(menu);

	QMenu *menuEx = new QMenu();
	menuEx->addAction(tr("Linux Configuration"), this, SLOT(exportLinux()));
	ui->exportBtn->setMenu(menuEx);

}

PresetDialog::~PresetDialog()
{
	delete ui;
}

void PresetDialog::reject()
{
	QDialog::reject();
}

void PresetDialog::presetIndexChanged()
{
	if (ui->files->currentItem() == nullptr)
	{
		return;
	}

	ui->presetName->setText(ui->files->currentItem()->text());
}

void PresetDialog::UpdateList()
{
	ui->files->clear();

	QString path = AppConfig::instance().getPath("presets");
	QDir    dir(path);

	if (!dir.exists())
	{
		dir.mkpath(".");
	}

	QStringList nameFilter("*.conf");
	QStringList files = dir.entryList(nameFilter);

	if (files.count() < 1)
	{
		QFont            font;
		font.setItalic(true);
		font.setPointSize(11);

		QListWidgetItem *placeholder = new QListWidgetItem;
		placeholder->setFont(font);
		placeholder->setText(tr("No presets saved"));
		placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
		ui->files->addItem(placeholder);
		return;
	}

	for (int i = 0; i < files.count(); i++)  // Strip extensions
	{
		QFileInfo fi(files[i]);
		files[i] = fi.completeBaseName();
	}

	ui->files->addItems(files);
}

void PresetDialog::add()
{
	if (ui->presetName->text() == "")
	{
		QMessageBox::warning(this, tr("Error"), tr("Preset Name is empty"), QMessageBox::Ok);
		return;
	}

	// TODO: Is this still neccessary? m_mainwin->applyConfig(false);

	QString path = AppConfig::instance().getPath("presets");
	MainWindow::savePresetFile(path + "/" + ui->presetName->text() + ".conf");
	ui->presetName->text() = "";
	UpdateList();
	emit    presetChanged();
}

void PresetDialog::importLinux()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Load custom audio.conf"), "", "*.conf");

	if (filename == "")
	{
		return;
	}

	QFileInfo      fileInfo(filename);
	QString        path = AppConfig::instance().getPath("presets");

	const QString &src  = filename;
	const QString  dest = path + "/" + fileInfo.fileName();

	if (QFile::exists(dest))
	{
		QFile::remove(dest);
	}

	QFile::copy(src, dest);
	Log::debug("Importing from " + filename + " (presets/linuximport)");
}

void PresetDialog::exportLinux()
{
	if (ui->files->selectedItems().length() == 0)
	{
		QMessageBox::warning(this, tr("Error"), tr("Nothing selected"), QMessageBox::Ok);
		return;
	}

	QString filename = QFileDialog::getSaveFileName(this, tr("Save audio.conf"), "", "*.conf");

	if (filename == "")
	{
		return;
	}

	QFileInfo fi(filename);
	QString   ext = fi.suffix();

	if (ext != "conf")
	{
		filename.append(".conf");
	}

	QFileInfo fileInfo(filename);
	QString   path     = AppConfig::instance().getPath("presets");
	QString   fullpath = QDir(path).filePath(ui->files->selectedItems().first()->text() + ".conf");
	QFile     file(fullpath);

	if (!QFile::exists(fullpath))
	{
		QMessageBox::warning(this, tr("Error"), tr("Selected file doesn't exist"), QMessageBox::Ok);
		UpdateList();
		emit presetChanged();
		return;
	}

	const QString &src  = fullpath;
	const QString  dest = filename;

	if (QFile::exists(dest))
	{
		QFile::remove(dest);
	}

	QFile::copy(src, dest);
	Log::debug("Exporting to " + filename + " (presets/linuxexport)");
}

void PresetDialog::remove()
{
	if (ui->files->selectedItems().length() == 0)
	{
		QMessageBox::warning(this, tr("Error"), tr("Nothing selected"), QMessageBox::Ok);
		return;
	}

	QString fullpath = AppConfig::instance().getPath("presets/" + ui->files->selectedItems().first()->text() + ".conf");
	QFile   file(fullpath);

	if (!QFile::exists(fullpath))
	{
		QMessageBox::warning(this, tr("Error"), tr("Selected file doesn't exist"), QMessageBox::Ok);
		UpdateList();
		emit presetChanged();
		return;
	}

	file.remove();
	Log::debug("Removed " + fullpath + " (presets/remove)");
	UpdateList();
	emit presetChanged();
}

void PresetDialog::load()
{
	if (ui->files->selectedItems().length() == 0)
	{
		QMessageBox::warning(this, tr("Error"), tr("Nothing selected"), QMessageBox::Ok);
		return;
	}

	QString fullpath = AppConfig::instance().getPath("presets/" + ui->files->selectedItems().first()->text() + ".conf");

	if (!QFile::exists(fullpath))
	{
		QMessageBox::warning(this, tr("Error"), tr("Selected File doesn't exist"), QMessageBox::Ok);
		UpdateList();
		emit presetChanged();
		return;
	}

	MainWindow::loadPresetFile(fullpath);
}

void PresetDialog::nameChanged(const QString &name)
{
	QString path = AppConfig::instance().getPath("presets");

	if (QFile::exists(path + "/" + name + ".conf"))
	{
		ui->add->setText(tr("Overwrite"));
	}
	else
	{
		ui->add->setText(tr("Save"));
	}
}

void PresetDialog::showContextMenu(const QPoint &pos)
{
	QPoint           globalPos     = ui->files->mapToGlobal(pos);
	QMenu            menu;
	QAction         *action_rename = menu.addAction(tr("Rename"));
	QAction         *action_del    = menu.addAction(tr("Delete"));
	QListWidgetItem *pointedItem   = ui->files->itemAt(pos);

	if (!pointedItem)
	{
		return;
	}

	QString  path     = AppConfig::instance().getPath("presets");
	QString  fullpath = QDir(path).filePath(pointedItem->text() + ".conf");
	QAction *selectedAction;

	if (pointedItem)
	{
		selectedAction = menu.exec(globalPos);

		if (selectedAction)
		{
			if (selectedAction == action_rename)
			{
				bool    ok;
				QString text = QInputDialog::getText(this, tr("Rename"),
				                                     tr("New Name"), QLineEdit::Normal,
				                                     pointedItem->text(), &ok);

				if (ok && !text.isEmpty())
				{
					QFile::rename(fullpath, QDir(path).filePath(text + ".conf"));
				}

				UpdateList();
				emit presetChanged();
			}

			if (selectedAction == action_del)
			{
				if (!QFile::exists(fullpath))
				{
					QMessageBox::warning(this, tr("Error"), tr("Selected file doesn't exist"), QMessageBox::Ok);
					UpdateList();
					emit presetChanged();
					return;
				}

				QFile::remove(fullpath);
				Log::debug("Removed " + fullpath);
				UpdateList();
				emit presetChanged();

			}
		}
	}

	menu.exec(globalPos);
}
