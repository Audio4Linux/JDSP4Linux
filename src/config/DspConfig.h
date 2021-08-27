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
#ifndef DSPCONFIGWRAPPER_H
#define DSPCONFIGWRAPPER_H

#include "AppConfig.h"
#include "ConfigContainer.h"
#include "ConfigIO.h"
#include "utils/Common.h"
#include "utils/Log.h"

#include <QObject>

using namespace std;

class DspConfig :
	public QObject
{
	Q_OBJECT

public:
	static DspConfig &instance()
	{
		static DspConfig _instance;
		return _instance;
	}

	DspConfig(DspConfig const &)       = delete;
	void operator= (DspConfig const &) = delete;

	DspConfig()
	{
		_conf = new ConfigContainer();
	}

	void set(const QString & key,
	         const QVariant &value)
	{
		_conf->setValue(key, value);
	}

	template<class T>
	T get(const QString &key)
	{
		if constexpr (std::is_same_v<T, QString>) {
			return _conf->getString(key, false);
		}
		else
		{
			if constexpr (std::is_same_v<T, int>) {
				return _conf->getInt(key, false);
			}
			else
			{
				if constexpr (std::is_same_v<T, float>) {
					return _conf->getFloat(key, false);
				}
				else
				{
					if constexpr (std::is_same_v<T, bool>) {
						return _conf->getBool(key, false);
					}
					else
					{
						Log::error("DspConfig::get<T>: Unknown type T");
						return 0;
					}
				}
			}
		}
	}

	void save()
	{
		auto file = AppConfig::instance().getDspConfPath();
		ConfigIO::writeFile(file, _conf->getConfigMap());
	}

	void load()
	{
		auto map = ConfigIO::readFile(AppConfig::instance().getDspConfPath());

		if (map.count() < 1)
		{
			Log::debug("DspConfig::load: Empty config file, using defaults");
			loadDefault();
			return;
		}

		_conf->setConfigMap(map);
		emit configBuffered();
	}

	void load(const QString &string)
	{
		auto map = ConfigIO::readString(string);

		if (map.count() < 1)
		{
			Log::debug("DspConfig::load: Empty config file, using defaults");
			loadDefault();
			return;
		}

		_conf->setConfigMap(map);
		emit configBuffered();
	}

	void loadDefault()
	{
		QString data;
		QString fileName(":/assets/default.conf");

		QFile   file(fileName);

		if (file.open(QIODevice::ReadOnly))
		{
			auto map = ConfigIO::readString(file.readAll());
			_conf->setConfigMap(map);
			emit configBuffered();
		}

	}

signals:
	void configBuffered();

private:
	ConfigContainer *_conf;
};

#endif // DSPCONFIGWRAPPER_H