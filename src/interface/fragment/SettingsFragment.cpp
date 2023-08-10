#include <IAudioService.h>

#include "SettingsFragment.h"
#include "ui_MainWindow.h"
#include "ui_SettingsFragment.h"

#include "config/AppConfig.h"
#include "data/AssetManager.h"
#include "interface/dialog/PaletteEditor.h"
#include "interface/QMenuEditor.h"
#include "interface/TrayIcon.h"
#include "MainWindow.h"
#include "utils/AutoStartManager.h"
#include "utils/DesktopServices.h"

#include <AeqSelector.h>

#include <AeqPackageManager.h>
#include <QCloseEvent>
#include <QDebug>
#include <QGraphicsOpacityEffect>
#include <QMessageBox>
#include <QProcess>
#include <QStyleFactory>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>
using namespace std;

SettingsFragment::SettingsFragment(TrayIcon *trayIcon,
                                   IAudioService *audioService,
                                   AutostartManager *autostart,
                                   QWidget  *parent) :
    BaseFragment(parent),
	ui(new Ui::SettingsFragment),
    _trayIcon(trayIcon),
    _autostart(autostart),
    _audioService(audioService),
    _paletteEditor(new PaletteEditor(&AppConfig::instance(), this))
{
	ui->setupUi(this);

#ifdef USE_PULSEAUDIO
    ui->selector->topLevelItem(3)->setHidden(true); // Hide devices
    ui->devices->setVisible(false);
    ui->blocklistBox->setVisible(false);
    ui->benchmarkOnBoot->setEnabled(false);
#endif

    _paletteEditor->setFixedSize(_paletteEditor->geometry().width(), _paletteEditor->geometry().height());

    /*
	 * Prepare TreeView
	 */
    ui->selector->setCurrentItem(ui->selector->topLevelItem(0));
	ui->stackedWidget->setCurrentIndex(0);
    ui->stackedWidget->repaint();
    ui->selector->expandItem(ui->selector->topLevelItem(5)); // Expand tray-icon
    connect(ui->selector, &QTreeWidget::currentItemChanged, this, &SettingsFragment::onTreeItemSelected);

	/*
	 * Prepare all combooxes
	 */
	ui->paletteSelect->addItem("Default",    "default");
	ui->paletteSelect->addItem("Black",      "black");
    ui->paletteSelect->addItem("Blue Neon",  "blue");
	ui->paletteSelect->addItem("Dark",       "dark");
	ui->paletteSelect->addItem("Dark Blue",  "darkblue");
	ui->paletteSelect->addItem("Dark Green", "darkgreen");
    ui->paletteSelect->addItem("Honeycomb",  "honeycomb");
	ui->paletteSelect->addItem("Green",      "green");
	ui->paletteSelect->addItem("Stone",      "stone");
	ui->paletteSelect->addItem("Custom",     "custom");

    for ( const auto& i : QStyleFactory::keys())
	{
		ui->themeSelect->addItem(i);
	}

    /*
     * Session signals
	 */
    connect(ui->systray_r_none, &QRadioButton::clicked, this, &SettingsFragment::onSystrayToggled);
    connect(ui->systray_r_showtray, &QRadioButton::clicked, this, &SettingsFragment::onSystrayToggled);
    connect(ui->systray_minOnBoot, &QPushButton::clicked, this, &SettingsFragment::onAutoStartToggled);
    connect(ui->menu_edit, &QMenuEditor::targetChanged, this, &SettingsFragment::onTrayEditorCommitted);
    connect(ui->menu_edit, &QMenuEditor::resetPressed, this, &SettingsFragment::onTrayEditorReset);
    ui->menu_edit->setSourceMenu(trayIcon->buildAvailableActions());

    /*
     * Interface signals
     */
    connect(ui->themeSelect, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsFragment::onThemeSelected);
    connect(ui->paletteSelect, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsFragment::onPaletteSelected);
    connect(ui->paletteConfig, &QPushButton::clicked, _paletteEditor, &PaletteEditor::show);
    connect(ui->eq_alwaysdrawhandles, &QCheckBox::clicked, this, &SettingsFragment::onEqualizerHandlesToggled);

    /*
     * Audio processing
     */
    connect(ui->benchmarkOnBoot, &QCheckBox::clicked, this, &SettingsFragment::onBenchmarkOnBootToggled);
    connect(ui->benchmarkNow, &QPushButton::clicked, this, &SettingsFragment::onBenchmarkRunClicked);
    connect(ui->benchmarkClear, &QPushButton::clicked, this, &SettingsFragment::onBenchmarkClearClicked);
    connect(ui->sinkAllowVolumeControl, &QCheckBox::clicked, this, &SettingsFragment::onSinkAllowVolumeControlClicked);
    connect(_audioService, &IAudioService::benchmarkDone, this, [this]{ updateBenchmarkStatus(tr("benchmark data loaded")); });

	/*
     * Paths signals
	 */
    connect(ui->liveprog_autoextract, &QCheckBox::clicked, this, &SettingsFragment::onLiveprogAutoExtractToggled);
    connect(ui->liveprog_extractNow, &QPushButton::clicked, this, &SettingsFragment::onExtractAssetsClicked);
    connect(ui->savePaths, &QPushButton::clicked, this, &SettingsFragment::onSavePathsClicked);

    /*
     * Network signals
     */
    connect(ui->aeqManage, &QPushButton::clicked, this, &SettingsFragment::onAeqDatabaseManageClicked);

	/*
     *  Devices signals
	 */
    connect(ui->dev_mode_auto, &QRadioButton::clicked, this, &SettingsFragment::onDefaultDeviceSelected);
    connect(ui->dev_mode_manual, &QRadioButton::clicked, this, &SettingsFragment::onDefaultDeviceSelected);
    connect(ui->dev_select, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsFragment::onDefaultDeviceSelected);

	/*
     * Global signals
	 */
    connect(ui->close, &QPushButton::clicked, this, &SettingsFragment::closePressed);
    connect(ui->github, &QPushButton::clicked, this, &SettingsFragment::onGithubLinkClicked);

    connect(ui->run_first_launch, &QPushButton::clicked, this, &SettingsFragment::onSetupWizardLaunchClicked);
    connect(ui->blocklistClear, &QPushButton::clicked, this, &SettingsFragment::onBlocklistClearClicked);
    connect(ui->blocklistInvert, &QCheckBox::stateChanged, this, &SettingsFragment::onBlocklistInvertToggled);

    connect(&AppConfig::instance(), &AppConfig::updated, this, &SettingsFragment::onAppConfigUpdated);

    /*
     * Refresh all input fields
     */
    refreshAll();

	/*
	 * Check for systray availability
	 */
    ui->systray_unsupported->hide();
#ifndef QT_NO_SYSTEMTRAYICON
	if (!QSystemTrayIcon::isSystemTrayAvailable())
	{
#endif
		ui->systray_unsupported->show();
		ui->session->setEnabled(false);
        ui->session_menu->setEnabled(false);
#ifndef QT_NO_SYSTEMTRAYICON
	}
#endif
}

SettingsFragment::~SettingsFragment()
{
    disconnect(&AppConfig::instance(), &AppConfig::updated, this, &SettingsFragment::onAppConfigUpdated);
	delete ui;
}

void SettingsFragment::onAppConfigUpdated(const AppConfig::Key &key, const QVariant &value)
{
    switch(key)
    {
        case AppConfig::AutoStartEnabled:
            ui->systray_minOnBoot->setChecked(value.toBool());
        default:
            break;
        }
}

void SettingsFragment::updateBenchmarkStatus(const QString &message)
{
    ui->benchmarkStatus->setText(QString("Status: %1").arg(message));
}

void SettingsFragment::refreshDevices()
{
    _lockslot = true;
	ui->dev_select->clear();

    ui->dev_mode_auto->setChecked(AppConfig::instance().get<bool>(AppConfig::AudioOutputUseDefault));
    ui->dev_mode_manual->setChecked(!AppConfig::instance().get<bool>(AppConfig::AudioOutputUseDefault));
    ui->dev_select->setDisabled(ui->dev_mode_auto->isChecked());

    auto devices = _audioService->sinkDevices();

    ui->dev_select->addItem("...", 0);
    for (const auto& device : devices)
    {
        ui->dev_select->addItem(QString("%1 (%2)")
                                .arg(QString::fromStdString(device.description))
                                .arg(QString::fromStdString(device.name)), QString::fromStdString(device.name));
    }

    auto current = AppConfig::instance().get<QString>(AppConfig::AudioOutputDevice);

    bool notFound = true;

    for (int i = 0; i < ui->dev_select->count(); i++)
    {
        if (ui->dev_select->itemData(i) == current)
        {
            notFound = false;
            ui->dev_select->setCurrentIndex(i);
            break;
        }
    }

    if (notFound)
    {
        QString name = QString(tr("Unknown (%1)")).arg(current);
        ui->dev_select->addItem(name, current);
        ui->dev_select->setCurrentText(name);
    }
    _lockslot = false;
}

void SettingsFragment::refreshAll()
{
    _lockslot = true;

    ui->menu_edit->setTargetMenu(_trayIcon->getTrayMenu());
	ui->irspath->setText(AppConfig::instance().getIrsPath());
    ui->ddcpath->setText(AppConfig::instance().getVdcPath());
	ui->liveprog_path->setText(AppConfig::instance().getLiveprogPath());

    ui->liveprog_autoextract->setChecked(AppConfig::instance().get<bool>(AppConfig::LiveprogAutoExtract));

    QString qvT(AppConfig::instance().get<QString>(AppConfig::Theme));
	int     indexT = ui->themeSelect->findText(qvT);

	if ( indexT != -1 )
	{
		ui->themeSelect->setCurrentIndex(indexT);
	}
	else
	{
		int index_fallback = ui->themeSelect->findText("Fusion");

		if ( index_fallback != -1 )
		{
			ui->themeSelect->setCurrentIndex(index_fallback);
		}
	}

    QVariant qvS2(AppConfig::instance().get<QString>(AppConfig::ThemeColors));
	int      index2 = ui->paletteSelect->findData(qvS2);

	if ( index2 != -1 )
	{
		ui->paletteSelect->setCurrentIndex(index2);
	}

    ui->paletteConfig->setEnabled(AppConfig::instance().get<QString>(AppConfig::ThemeColors) == "custom");

    ui->systray_r_none->setChecked(!AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->systray_r_showtray->setChecked(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->systray_icon_box->setEnabled(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));
    ui->menu_edit->setEnabled(AppConfig::instance().get<bool>(AppConfig::TrayIconEnabled));

    ui->systray_minOnBoot->setChecked(AppConfig::instance().get<bool>(AppConfig::AutoStartEnabled));

    ui->eq_alwaysdrawhandles->setChecked(AppConfig::instance().get<bool>(AppConfig::EqualizerShowHandles));

    ui->benchmarkOnBoot->setChecked(AppConfig::instance().get<bool>(AppConfig::BenchmarkOnBoot));
    ui->sinkAllowVolumeControl->setChecked(!AppConfig::instance().get<bool>(AppConfig::AudioVirtualSinkForceMaxValue));

    ui->blocklistInvert->blockSignals(true);
    ui->blocklistInvert->setChecked(AppConfig::instance().get<bool>(AppConfig::AudioAppBlocklistInvert));
    ui->blocklistInvert->blockSignals(false);

    ui->aeqStatus->setText(AeqPackageManager().isPackageInstalled() ? tr("installed") : tr("not installed"));

    if(AppConfig::instance().get<QString>(AppConfig::BenchmarkCacheC0).isEmpty() &&
       AppConfig::instance().get<QString>(AppConfig::BenchmarkCacheC1).isEmpty())
        updateBenchmarkStatus(tr("no benchmark data stored"));
    else
        updateBenchmarkStatus(tr("benchmark data loaded"));

    refreshDevices();

    _lockslot = false;
}

void SettingsFragment::updateButtonStyle(bool white)
{
    ui->menu_edit->setIconStyle(white);
}

void SettingsFragment::onSavePathsClicked()
{
    AppConfig::instance().setIrsPath(ui->irspath->text());
    AppConfig::instance().setVdcPath(ui->ddcpath->text());
    AppConfig::instance().setLiveprogPath(ui->liveprog_path->text());
}

void SettingsFragment::onExtractAssetsClicked()
{
    auto result = QMessageBox::question(this, tr("Override liveprog scripts?"),
                                        tr("Do you want to override existing default liveprog scripts?\nIf they have been modified, they will be reset."));

    int i = AssetManager::instance().extractAll(result == QMessageBox::Yes);
    QMessageBox::information(this, tr("Extract assets"), tr("%1 files have been restored").arg(i));
}

void SettingsFragment::onDefaultDeviceSelected()
{
    if (_lockslot)
    {
        return;
    }

    ui->dev_select->setDisabled(ui->dev_mode_auto->isChecked());
    AppConfig::instance().set(AppConfig::AudioOutputUseDefault, ui->dev_mode_auto->isChecked());

    if (!ui->dev_mode_auto->isChecked())
    {
        if (ui->dev_select->currentData() == "...")
        {
            return;
        }

        AppConfig::instance().set(AppConfig::AudioOutputDevice, ui->dev_select->currentData());
    }
}

void SettingsFragment::onTreeItemSelected(QTreeWidgetItem *cur, QTreeWidgetItem *prev)
{
    Q_UNUSED(prev)
    int topLevelIndex = ui->selector->indexOfTopLevelItem(cur);

    switch (topLevelIndex)
    {
        case -1:
            if (ui->selector->indexOfTopLevelItem(cur->parent()) == 5 /* Tray-icon */)
            {
                ui->stackedWidget->setCurrentIndex(6); // Context menu
            }
            break;
        default:
            ui->stackedWidget->setCurrentIndex(topLevelIndex);
            break;
    }

    // Workaround: Force redraw
    ui->stackedWidget->hide();
    ui->stackedWidget->show();
    ui->stackedWidget->repaint();
}

void SettingsFragment::onAutoStartToggled()
{
    _autostart->setEnabled(ui->systray_minOnBoot->isChecked());
}

void SettingsFragment::onSystrayToggled()
{
    if (_lockslot)
    {
        return;
    }

    AppConfig::instance().set(AppConfig::TrayIconEnabled, ui->systray_r_showtray->isChecked());
    ui->systray_icon_box->setEnabled(ui->systray_r_showtray->isChecked());
    ui->menu_edit->setEnabled(ui->systray_r_showtray->isChecked());
    if(!ui->systray_r_showtray->isChecked()) {
        ui->systray_minOnBoot->setChecked(false);
        onAutoStartToggled();
    }
}

void SettingsFragment::onThemeSelected(int index)
{
    if (_lockslot)
    {
        return;
    }

    AppConfig::instance().set(AppConfig::Theme, ui->themeSelect->itemText(index).toUtf8().constData());
}

void SettingsFragment::onPaletteSelected(int index)
{
    if (_lockslot)
    {
        return;
    }

    AppConfig::instance().set(AppConfig::ThemeColors, ui->paletteSelect->itemData(index).toString());
    ui->paletteConfig->setEnabled(AppConfig::instance().get<QString>(AppConfig::ThemeColors) == "custom");
}

void SettingsFragment::onBlocklistInvertToggled(bool state)
{
    auto invertInfo = tr("You are about to enable allowlist mode. JamesDSP will not process all applications by default while this mode is active. "
                      "You need to explicitly allow each app to get processed in the 'Apps' menu.\n");
    auto button = QMessageBox::question(this, tr("Are you sure?"),
                          state ? invertInfo : tr("This action will reset your current blocklist or allowlist. Do you want to continue?"));

    if(button == QMessageBox::Yes)
    {
        AppConfig::instance().set(AppConfig::AudioAppBlocklistInvert, state);
        AppConfig::instance().set(AppConfig::AudioAppBlocklist, "");
    }
}

void SettingsFragment::onBlocklistClearClicked()
{
    AppConfig::instance().set(AppConfig::AudioAppBlocklist, "");
}

void SettingsFragment::onSetupWizardLaunchClicked()
{
    emit closePressed();
    QTimer::singleShot(300, this, &SettingsFragment::launchSetupWizard);
}

void SettingsFragment::onTrayEditorCommitted()
{
    auto menu = ui->menu_edit->exportMenu();
    _trayIcon->updateTrayMenu(menu);
}

void SettingsFragment::onTrayEditorReset()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Warning"), tr("Do you really want to restore the default menu layout?"),
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes)
    {
        ui->menu_edit->setTargetMenu(_trayIcon->buildDefaultActions());
        _trayIcon->updateTrayMenu(ui->menu_edit->exportMenu());
    }
}

void SettingsFragment::onEqualizerHandlesToggled()
{
    AppConfig::instance().set(AppConfig::EqualizerShowHandles, ui->eq_alwaysdrawhandles->isChecked());
}

void SettingsFragment::onBenchmarkOnBootToggled()
{
    AppConfig::instance().set(AppConfig::BenchmarkOnBoot, ui->benchmarkOnBoot->isChecked());
}

void SettingsFragment::onBenchmarkRunClicked()
{
    _audioService->runBenchmarks();
    updateBenchmarkStatus(tr("waiting for result..."));
}

void SettingsFragment::onBenchmarkClearClicked()
{
    AppConfig::instance().set(AppConfig::BenchmarkCacheC0, "");
    AppConfig::instance().set(AppConfig::BenchmarkCacheC1, "");
    QMessageBox::information(this, tr("Cache cleared"), tr("Benchmark data has been cleared. Restart this app to fully apply the changes."));
    updateBenchmarkStatus(tr("no benchmark data stored"));
}

void SettingsFragment::onSinkAllowVolumeControlClicked()
{
    AppConfig::instance().set(AppConfig::AudioVirtualSinkForceMaxValue, !ui->sinkAllowVolumeControl->isChecked());
}

void SettingsFragment::onLiveprogAutoExtractToggled()
{
    AppConfig::instance().set(AppConfig::LiveprogAutoExtract, ui->liveprog_autoextract->isChecked());
}

void SettingsFragment::onGithubLinkClicked()
{
    DesktopServices::openUrl("https://github.com/Audio4Linux/JDSP4Linux", this);
}

void SettingsFragment::onAeqDatabaseManageClicked()
{
    auto* aeqSel = new AeqSelector(this);
    aeqSel->forceManageMode();
    aeqSel->exec();
    refreshAll();
}

void SettingsFragment::setVisible(bool visible)
{
    refreshAll();
    BaseFragment::setVisible(visible);
}
