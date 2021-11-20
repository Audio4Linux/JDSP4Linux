#include "PresetFragment.h"
#include "ui_PresetFragment.h"

#include "config/AppConfig.h"
#include "data/PresetManager.h"
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

PresetFragment::PresetFragment(QWidget *parent) :
    BaseFragment(parent),
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

    connect(ui->close, &QPushButton::clicked, this, &PresetFragment::closePressed);
}

PresetFragment::~PresetFragment()
{
	delete ui;
}

void PresetFragment::presetIndexChanged()
{
	if (ui->files->currentItem() == nullptr)
	{
		return;
	}

	ui->presetName->setText(ui->files->currentItem()->text());
}

void PresetFragment::UpdateList()
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
		QListWidgetItem *placeholder = new QListWidgetItem;
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

void PresetFragment::add()
{
	if (ui->presetName->text() == "")
	{
		QMessageBox::warning(this, tr("Error"), tr("Preset Name is empty"), QMessageBox::Ok);
		return;
	}

    emit wantsToWriteConfig();

	QString path = AppConfig::instance().getPath("presets");
    PresetManager::instance().save(path + "/" + ui->presetName->text() + ".conf");

	ui->presetName->text() = "";
	UpdateList();
    emit presetChanged();
}

void PresetFragment::remove()
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
    Log::debug("PresetDialog::remove: Deleted " + fullpath);
	UpdateList();
	emit presetChanged();
}

void PresetFragment::load()
{
	if (ui->files->selectedItems().length() == 0)
	{
		QMessageBox::warning(this, tr("Error"), tr("Nothing selected"), QMessageBox::Ok);
		return;
	}

	QString fullpath = AppConfig::instance().getPath("presets/" + ui->files->selectedItems().first()->text() + ".conf");

	if (!QFile::exists(fullpath))
	{
        QMessageBox::warning(this, tr("Error"), tr("Selected file doesn't exist anymore"), QMessageBox::Ok);
		UpdateList();
		emit presetChanged();
		return;
	}

    PresetManager::instance().load(fullpath);
}

void PresetFragment::nameChanged(const QString &name)
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

void PresetFragment::showContextMenu(const QPoint &pos)
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
                    QMessageBox::warning(this, tr("Error"), tr("Selected file doesn't exist anymore"), QMessageBox::Ok);
					UpdateList();
					emit presetChanged();
					return;
				}

				QFile::remove(fullpath);
                Log::debug("PresetDialog::showContextMenu: Deleted via context menu: " + fullpath);
				UpdateList();
				emit presetChanged();

			}
		}
	}
}
