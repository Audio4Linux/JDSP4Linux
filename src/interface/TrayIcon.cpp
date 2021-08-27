#include "TrayIcon.h"

#include "config/AppConfig.h"
#include "data/PresetProvider.h"
#include "utils/Log.h"

#include <QApplication>
#include <QMenu>

TrayIcon::TrayIcon(QObject *parent) : QObject(parent)
{
	createTrayIcon();

	tray_disableAction = new QAction(tr("&Disable FX"), this);
	tray_disableAction->setProperty("tag", "disablefx");
	tray_disableAction->setCheckable(true);
	tray_disableAction->setChecked(false);
	connect(tray_disableAction, &QAction::toggled, this, &TrayIcon::changeDisableFx);

	tray_presetMenu = new QMenu(tr("&Presets"));
	tray_presetMenu->setProperty("tag", "menu_preset");

	tray_convMenu   = new QMenu(tr("&Convolver Bookmarks"));
	tray_convMenu->setProperty("tag", "menu_convolver");

	auto init = MenuIO::buildMenu(buildAvailableActions(), AppConfig::instance().getTrayContextMenu(), menuOwner);

	if (init->actions().count() < 1)
	{
		init = buildDefaultActions();
	}

	updateTrayMenu(init);
}

void TrayIcon::createTrayIcon()
{
	menuOwner = new QWidget();
	trayIcon  = new QSystemTrayIcon(this);
	trayIcon->setToolTip("JamesDSP for Linux");
	connect(trayIcon, &QSystemTrayIcon::activated, this, &TrayIcon::iconActivated);
	trayIcon->setIcon(QIcon(":/icons/icon.png"));
}

void TrayIcon::setTrayVisible(bool visible)
{
	if (visible)
	{
		trayIcon->show();
	}
	else
	{
		trayIcon->hide();
	}
}

void TrayIcon::iconEventHandler(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
		case QSystemTrayIcon::Trigger:
			emit iconActivated();
			break;
		default:
			break;
	}
}

void TrayIcon::changedDisableFx(bool disabled)
{
	tray_disableAction->setChecked(disabled);
}

void TrayIcon::updatePresetList()
{
	if (tray_presetMenu != nullptr)
	{
		tray_presetMenu->clear();
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
			QAction *noPresets = new QAction("No presets found");
			noPresets->setEnabled(false);
			tray_presetMenu->addAction(noPresets);
		}
		else
		{
			for (int i = 0; i < files.count(); i++)
			{
				// Strip extensions
				QFileInfo fi(files[i]);
				files[i] = fi.completeBaseName();

				// Add entry
				QAction  *newEntry = new QAction(files[i]);
				connect(newEntry, &QAction::triggered, this, [ = ]()
				{
					emit loadPreset(AppConfig::instance().getPath(QString("presets/%1").arg(files[i])));
				});

				tray_presetMenu->addAction(newEntry);
			}
		}
	}
}

void TrayIcon::updateConvolverList()
{
	if (tray_convMenu != nullptr)
	{
		tray_convMenu->clear();
		QString path = AppConfig::instance().getPath("irs_favorites");

		QDir    dir(path);

		if (!dir.exists())
		{
			dir.mkpath(".");
		}

		QStringList nameFilter({ "*.wav", "*.irs" });
		QStringList files = dir.entryList(nameFilter);

		if (files.count() < 1)
		{
			QAction *noPresets = new QAction("No impulse responses found");
			noPresets->setEnabled(false);
			tray_convMenu->addAction(noPresets);
		}
		else
		{
			for (int i = 0; i < files.count(); i++)
			{
				// Add entry
				QAction *newEntry = new QAction(files[i]);
				connect(newEntry, &QAction::triggered, this, [ = ]()
				{
					emit loadIrs(files[i]);
				});
				tray_convMenu->addAction(newEntry);
			}
		}
	}
}

void TrayIcon::updateTrayMenu(QMenu *menu)
{
	if (menu == nullptr)
	{
		Log::error("TrayIcon::updateTrayMenu: menu is nullptr");
		return;
	}

	trayIcon->hide();
	trayIcon->deleteLater();
	// menuOwner->deleteLater();
	createTrayIcon();

	trayIcon->show();
	trayIcon->setContextMenu(menu);
	connect(trayIcon->contextMenu(), &QMenu::aboutToShow, [this]
	{
		updatePresetList();
		updateConvolverList();
	});

	AppConfig::instance().setTrayContextMenu(MenuIO::buildString(menu));
}

bool TrayIcon::isVisible() const
{
	return trayIcon->isVisible();
}

QMenu* TrayIcon::buildAvailableActions()
{
	QAction *reloadAction = new QAction(tr("&Reload JamesDSP"), this);
	reloadAction->setProperty("tag", "reload");
	connect(reloadAction, &QAction::triggered, this, &TrayIcon::restart);

	QAction *quitAction = new QAction(tr("&Quit"), this);
	connect(quitAction,   &QAction::triggered, qApp, &QCoreApplication::quit);
	quitAction->setProperty("tag", "quit");

	QAction *showWindowAction = new QAction(tr("&Show/hide window"), this);
	connect(showWindowAction, &QAction::triggered, this, &TrayIcon::iconActivated);
	showWindowAction->setProperty("tag", "show");

	QMenu   *reverbMenu = new QMenu(tr("Re&verberation Presets"));

	for (auto preset : PresetProvider::Reverb::getPresetNames())
	{
		QAction *newEntry = new QAction(preset);
		connect(newEntry, &QAction::triggered, this, [ = ]() {
			emit loadReverbPreset(preset);
		});
		reverbMenu->addAction(newEntry);
	}

	reverbMenu->setProperty("tag", "menu_reverb_preset");

	QMenu *eqMenu = new QMenu(tr("&EQ Presets"), menuOwner);

	for (auto preset : PresetProvider::EQ::EQ_LOOKUP_TABLE().keys())
	{
		QAction *newEntry = new QAction(preset);
		connect(newEntry, &QAction::triggered, this, [ = ]() {
			emit loadEqPreset(preset);
		});
		eqMenu->addAction(newEntry);
	}

	eqMenu->setProperty("tag", "menu_eq_preset");

	QMenu *bs2bMenu = new QMenu(tr("&Crossfeed Presets"), menuOwner);

	for (auto preset : PresetProvider::BS2B::BS2B_LOOKUP_TABLE().toStdMap())
	{
		QAction *newEntry = new QAction(preset.first);
		connect(newEntry, &QAction::triggered, this, [ = ]() {
			emit loadCrossfeedPreset(preset.second);
		});
		bs2bMenu->addAction(newEntry);
	}

	bs2bMenu->setProperty("tag", "menu_crossfeed_preset");

	QMenu *menu = new QMenu(menuOwner);
	menu->addAction(tray_disableAction);
	menu->addAction(reloadAction);
	menu->addMenu(tray_presetMenu);
	menu->addSeparator();
	menu->addMenu(reverbMenu);
	menu->addMenu(eqMenu);
	menu->addMenu(bs2bMenu);
	menu->addMenu(tray_convMenu);
	menu->addSeparator();
	menu->addAction(showWindowAction);
	menu->addAction(quitAction);
	return menu;
}

QMenu* TrayIcon::buildDefaultActions()
{
	QAction *reloadAction = new QAction(tr("&Reload JamesDSP"), this);
	reloadAction->setProperty("tag", "reload");
	connect(reloadAction,     &QAction::triggered, this, &TrayIcon::restart);

	QAction *showWindowAction = new QAction(tr("&Show/hide window"), this);
	connect(showWindowAction, &QAction::triggered, this, &TrayIcon::iconActivated);
	showWindowAction->setProperty("tag", "show");

	QAction *quitAction = new QAction(tr("&Quit"), this);
	connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
	quitAction->setProperty("tag", "quit");

	QMenu   *menu = new QMenu();
	menu->addAction(tray_disableAction);
	menu->addAction(reloadAction);
	menu->addMenu(tray_presetMenu);
	menu->addSeparator();
	menu->addAction(showWindowAction);
	menu->addAction(quitAction);
	return menu;
}

QMenu* TrayIcon::getTrayMenu()
{
	return trayIcon->contextMenu();
}