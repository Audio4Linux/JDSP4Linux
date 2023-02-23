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
#ifndef PRESETEXTENSION_H
#define PRESETEXTENSION_H

#include <QMap>
#include <QString>
#include <QVector>

#define DOUBLE_LIST QVector<double>
#define FLOAT_LIST  QVector<float>
#define INT_LIST    QVector<int>

#define EQ_UNIT   QString, DOUBLE_LIST
#define BS2B_UNIT QString, int
#define SW_UNIT   QString, FLOAT_LIST

#define DEFAULT_GRAPHICEQ "GraphicEQ: 25 0; 40 0; 63 0; 100 0; 160 0; 250 0; 400 0; 630 0; 1000 0; 1600 0; 2500 0; 4000 0; 6300 0; 10000 0; 16000 0"

namespace PresetProvider
{
	class EQ
	{
public:
		static const DOUBLE_LIST   defaultPreset();
        static const QString       defaultPresetName();
		static const DOUBLE_LIST   lookupPreset(const QString &preset);
		static const QMap<EQ_UNIT> EQ_LOOKUP_TABLE();
		static const QString       reverseLookup(const QVector<double> &data);

	};

	class BS2B
	{
public:
        static const QMap<BS2B_UNIT> BS2B_LOOKUP_TABLE();
		static int                   lookupPreset(const QString &preset);
		static const QString         reverseLookup(int data);

	};

	class Reverb
	{
public:
		typedef enum
		{
			SF_REVERB_PRESET_DEFAULT,
			SF_REVERB_PRESET_SMALLHALL1,
			SF_REVERB_PRESET_SMALLHALL2,
			SF_REVERB_PRESET_MEDIUMHALL1,
			SF_REVERB_PRESET_MEDIUMHALL2,
			SF_REVERB_PRESET_LARGEHALL1,
			SF_REVERB_PRESET_LARGEHALL2,
			SF_REVERB_PRESET_SMALLROOM1,
			SF_REVERB_PRESET_SMALLROOM2,
			SF_REVERB_PRESET_MEDIUMROOM1,
			SF_REVERB_PRESET_MEDIUMROOM2,
			SF_REVERB_PRESET_LARGEROOM1,
			SF_REVERB_PRESET_LARGEROOM2,
			SF_REVERB_PRESET_MEDIUMER1,
			SF_REVERB_PRESET_MEDIUMER2,
			SF_REVERB_PRESET_PLATEHIGH,
			SF_REVERB_PRESET_PLATELOW,
			SF_REVERB_PRESET_LONGREVERB1,
			SF_REVERB_PRESET_LONGREVERB2
		} sf_reverb_preset;
		typedef struct
		{
			int    osf;
			double p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16;
		} sf_reverb_preset_data;
		static const sf_reverb_preset_data lookupPreset(int index);
		static const QStringList           getPresetNames();

	};

}
#endif // PRESETEXTENSION_H
