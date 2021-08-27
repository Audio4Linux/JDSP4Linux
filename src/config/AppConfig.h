/*
 *  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  ThePBone <tim.schneeberger(at)outlook.de> (c) 2020
 */
#ifndef APPCONFIGWRAPPER_H
#define APPCONFIGWRAPPER_H

#include "ConfigContainer.h"
#include "ConfigIO.h"
#include "utils/Common.h"

#include <QObject>

using namespace std;

class AppConfig :
	public QObject
{
	Q_OBJECT

public:
	static AppConfig &instance()
	{
		static AppConfig _instance;
		return _instance;
	}

	AppConfig(AppConfig const &)       = delete;
	void operator= (AppConfig const &) = delete;

	AppConfig()
	{
		_appconf = new ConfigContainer();
		load();
	}

#define DEF_BOOL(function, path, defaultValue, additionalSetCode) \
	bool get ## function() {                                      \
		return _appconf->getBool(path, true, defaultValue);       \
	}                                                             \
	void set ## function(bool val) {                              \
		_appconf->setValue(path, QVariant(val));                  \
		additionalSetCode                                         \
		save();                                                   \
	}
#define DEF_INT(function, path, defaultValue, additionalSetCode) \
	int get ## function() {                                      \
		return _appconf->getInt(path, true, defaultValue);       \
	}                                                            \
	void set ## function(int val) {                              \
		_appconf->setValue(path, QVariant(val));                 \
		additionalSetCode                                        \
		save();                                                  \
	}
#define DEF_FLOAT(function, path, defaultValue, additionalSetCode) \
	float get ## function() {                                      \
		return _appconf->getFloat(path, true, defaultValue);       \
	}                                                              \
	void set ## function(float val) {                              \
		_appconf->setValue(path, QVariant(val));                   \
		additionalSetCode                                          \
		save();                                                    \
	}
#define DEF_STRING(function, path, defaultValue, additionalSetCode) \
	QString get ## function() {                                     \
		return _appconf->getString(path, true, defaultValue);       \
	}                                                               \
	void set ## function(const QString &val) {                      \
		_appconf->setValue(path, QVariant(val));                    \
		additionalSetCode                                           \
		save();                                                     \
	}

#define DEF_STRING_NO_MOD(function, path)        \
	QString get ## function() {                  \
		return _appconf->getString(path, false); \
	}                                            \
	void set ## function(const QString &val) {   \
		_appconf->setValue(path, QVariant(val)); \
		save();                                  \
	}

	DEF_INT(AutoFxMode, "apply.auto.mode", 1, );
	DEF_BOOL(MuteOnRestart,       "apply.mutedreload",            false, );
	DEF_BOOL(GFix,                "apply.fixglava",               false, );

	DEF_BOOL(LiveprogAutoExtract, "liveprog.default.autoextract", false, );

	DEF_STRING(Theme,         "theme.name",           "", emit styleChanged(); );
	DEF_STRING(Stylesheet,    "theme.stylesheet",     "", emit styleChanged(); );
	DEF_STRING(Colorpalette,  "theme.palette",        "", emit styleChanged(); );
	DEF_STRING(Custompalette, "theme.palette.custom", "", emit styleChanged(); );
	DEF_BOOL(WhiteIcons, "theme.icons.white", false, emit styleChanged(); );

	DEF_INT(TrayMode, "session.tray.mode", 0, emit trayModeChanged(val); );
	DEF_STRING(TrayContextMenu, "session.tray.contextmenu", "", );

	DEF_BOOL(SpectrumEnable, "visualizer.spectrum.enable", false, emit spectrumChanged(); );
	DEF_BOOL(SpectrumGrid,   "visualizer.spectrum.grid",   false, emit spectrumChanged(); );
	DEF_INT(SpectrumBands,   "visualizer.spectrum.bands",         0, emit spectrumChanged(); );
	DEF_INT(SpectrumMinFreq, "visualizer.spectrum.frequency.min", 0, emit spectrumChanged(); );
	DEF_INT(SpectrumMaxFreq, "visualizer.spectrum.frequency.max", 0, emit spectrumChanged(); );
	DEF_INT(SpectrumShape,   "visualizer.spectrum.shape",         0, emit spectrumChanged(); );
	DEF_INT(SpectrumRefresh, "visualizer.spectrum.interval",      0, emit spectrumReloadRequired(); );
	DEF_FLOAT(SpectrumMultiplier, "visualizer.spectrum.multiplier", 0, emit spectrumChanged(); );

	DEF_BOOL(EqualizerPermanentHandles, "equalizer.handle.permanent", false, emit eqChanged(); );
	DEF_BOOL(IntroShown,                "app.firstlaunch",            false, );

	DEF_STRING(ExecutablePath, "app.executable", "", );
	DEF_STRING_NO_MOD(LastVdcDatabaseId, "vdc.database.lastid");


	void setSpectrumInput(const QString &npath)
	{
		_appconf->setValue("visualizer.spectrum.device", QVariant(npath));
		emit spectrumReloadRequired();
		save();
	}

	QString getSpectrumInput()
	{
		QString in = _appconf->getString("visualizer.spectrum.device", true);

		if (in.length() < 1)
		{
			QString defaultstr = "jdsp.monitor";
			setSpectrumInput(defaultstr);
			return defaultstr;
		}

		return in;
	}

	QString getDspConfPath()
	{
		return QString("%1/.config/jamesdsp/audio.conf").arg(QDir::homePath());
	}

	QString getPath(QString subdir = "")
	{
		return QString("%1/.config/jamesdsp/%2").arg(QDir::homePath()).arg(subdir);
	}

	void setIrsPath(const QString &npath)
	{
		_appconf->setValue("convolver.default.irspath", QVariant(QString("\"%1\"").arg(npath)));
		save();
	}

	QString getIrsPath()
	{
		QString irs_path = chopFirstLastChar(_appconf->getString("convolver.default.irspath", false));

		if (irs_path.length() < 2)
		{
			return QString("%1/IRS").arg(QDir::homePath());
		}

		return irs_path;
	}

	void setDDCPath(const QString &npath)
	{
		_appconf->setValue("ddc.default.path", QVariant(QString("\"%1\"").arg(npath)));
		save();
	}

	QString getDDCPath()
	{
		QString irs_path = chopFirstLastChar(_appconf->getString("ddc.default.path", false));

		if (irs_path.length() < 2)
		{
			return QString("%1/DDC").arg(QDir::homePath());
		}

		return irs_path;
	}

	void setLiveprogPath(const QString &npath)
	{
		_appconf->setValue("liveprog.default.path", QVariant(QString("\"%1\"").arg(npath)));
		save();
	}

	QString getLiveprogPath()
	{
		QString absolute = QFileInfo(getDspConfPath()).absoluteDir().absolutePath();
		QString lp_path  = chopFirstLastChar(_appconf->getString("liveprog.default.path", false));

		if (lp_path.length() < 2)
		{
			QDir(absolute).mkdir("liveprog");
			QString defaultstr = QString("%1/liveprog").arg(absolute);
			setLiveprogPath(defaultstr);
			return defaultstr;
		}

		return lp_path;
	}

	void save()
	{
		auto file = QString("%1/.config/jamesdsp/ui.2.conf").arg(QDir::homePath());
		ConfigIO::writeFile(file, _appconf->getConfigMap());
	}

	void load()
	{
		auto map = ConfigIO::readFile(QString("%1/.config/jamesdsp/ui.2.conf").arg(QDir::homePath()));
		_appconf->setConfigMap(map);
	}

	QString getGraphicEQConfigFilePath()
	{
		return pathAppend(QFileInfo(getDspConfPath()).absoluteDir().absolutePath(), "ui.graphiceq.conf");
	}

signals:
	void spectrumChanged();
	void spectrumReloadRequired();
	void styleChanged();
	void eqChanged();
	void trayModeChanged(bool);

private:
	ConfigContainer *_appconf;
};

#endif // APPCONFIGWRAPPER_H