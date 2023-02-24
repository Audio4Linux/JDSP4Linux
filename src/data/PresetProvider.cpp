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
#include "PresetProvider.h"

#include "InitializableQMap.h"
#include "utils/Common.h"

namespace PresetProvider
{
    const DOUBLE_LIST EQ::defaultPreset()
    {
        return EQ_LOOKUP_TABLE()[defaultPresetName()];
    }

    const QString EQ::defaultPresetName()
    {
        return QObject::tr("Flat");
    }

	const DOUBLE_LIST EQ::lookupPreset(const QString &preset)
	{
		auto table = EQ_LOOKUP_TABLE();

		if (table.contains(preset))
		{
			return table[preset];
		}
		else
		{
            return DOUBLE_LIST();
		}
	}

	const QString EQ::reverseLookup(const QVector<double> &data)
	{
		auto table = EQ_LOOKUP_TABLE();

		for (auto key : table.keys())
		{
			QVector<double> row(table[key]);
			int             it        = 0;
			bool            different = false;

			for (auto cur_data : row)
			{
				bool equal = isApproximatelyEqual<double>(cur_data, data.at(it), 0.01);

				if (!equal)
				{
					different = true;
					break;
				}

				it++;
			}

			if (!different)
			{
				return key;
			}
		}

        return "";
	}

	const QMap<EQ_UNIT> EQ::EQ_LOOKUP_TABLE()
	{
		InitializableQMap<EQ_UNIT> table;
        table << QPair<EQ_UNIT>(defaultPresetName(), DOUBLE_LIST({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }))
              << QPair<EQ_UNIT>(QObject::tr("Pop"), DOUBLE_LIST({ 0.0, 0.0, 0.0, 0.0, 0.0, 1.3, 2.0, 2.5, 5.0, -1.5, -2.0, -3.0, -3.0, -3.0, -3.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Rock"), DOUBLE_LIST({ 0.0, 0.0, 0.0, 3.0, 3.0, -10.0, -4.0, -1.0, 0.8, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Jazz"), DOUBLE_LIST({ 0.0, 0.0, 0.0, 2.0, 4.0, 5.9, -5.9, -4.5, -2.5, 2.5, 1.0, -0.8, -0.8, -0.8, -0.8 }))
              << QPair<EQ_UNIT>(QObject::tr("Classic"), DOUBLE_LIST({ -0.3, 0.3, -3.5, -9.0, -1.0, 0.0, 1.8, 2.1, 0.0, 0.0, 0.0, 4.4, 9.0, 9.0, 9.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Bass"), DOUBLE_LIST({ 10.00, 8.80, 8.50, 6.50, 2.50, 1.50, 0, 0, 0, 0, 0, 0, 0, 0, 0 }))
              << QPair<EQ_UNIT>(QObject::tr("Clear"), DOUBLE_LIST({ 3.5, 5.5, 6.5, 9.5, 8.0, 6.5, 3.5, 2.5, 1.3, 5.0, 7.0, 9.0, 10.0, 11.0, 9.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Hip-Hop"), DOUBLE_LIST({ 4.5, 4.3, 4.0, 2.5, 1.5, 3.0, -1.0, -1.5, -1.5, 1.5, 0.0, -1.0, 0.0, 1.5, 3.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Dubstep"), DOUBLE_LIST({ 12.0, 10.0, 0.5, -1.0, -3.0, -5.0, -5.0, -4.8, -4.5, -2.5, -1.0, 0.0, -2.5, -2.5, 0.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Movie"), DOUBLE_LIST({ 3.0, 3.0, 6.1, 8.5, 9.0, 7.0, 6.1, 6.1, 5.0, 8.0, 3.5, 3.5, 8.0, 10.0, 8.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Metal"), DOUBLE_LIST({ 10.5, 10.5, 7.5, 0.0, 2.0, 5.5, 0.0, 0.0, 0.0, 6.1, 0.0, 0.0, 6.1, 10.0, 12.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Vocal Booster"), DOUBLE_LIST({ -1.5, -2.0, -3.0, -3.0, -0.5, 1.5, 3.5, 3.5, 3.5, 3.0, 2.0, 1.5, 0.0, 0.0, -1.5 }))
              << QPair<EQ_UNIT>(QObject::tr("Hardstyle"), DOUBLE_LIST({ 6.1, 7.0, 12.0, 6.1, -5.0, -12.0, -2.5, 3.0, 6.5, 0.0, -2.2, -4.5, -6.1, -9.2, -10.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Acoustic"), DOUBLE_LIST({ 5.00, 4.50, 4.00, 3.50, 1.50, 1.00, 1.50, 1.50, 2.00, 3.00, 3.50, 4.00, 3.70, 3.00, 3.00 }))
              << QPair<EQ_UNIT>(QObject::tr("R&B"), DOUBLE_LIST({ 3.0, 3.0, 7.0, 6.1, 4.5, 1.5, -1.5, -2.0, -1.5, 2.0, 2.5, 3.0, 3.5, 3.8, 4.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Electronic"), DOUBLE_LIST({ 4.0, 4.0, 3.5, 1.0, 0.0, -0.5, -2.0, 0.0, 2.0, 0.0, 0.0, 1.0, 3.0, 4.0, 4.5 }))
              << QPair<EQ_UNIT>(QObject::tr("Deep Bass"), DOUBLE_LIST({ 12.0, 8.0, 0.0, -6.7, -12.0, -9.0, -3.5, -3.5, -6.1, 0.0, -3.0, -5.0, 0.0, 1.2, 3.0 }))
              << QPair<EQ_UNIT>(QObject::tr("Beats"), DOUBLE_LIST({ -5.5, -5.0, -4.5, -4.2, -3.5, -3.0, -1.9, 0, 0, 0, 0, 0, 0, 0, 0 }));
        return table;
	}

	const QMap<BS2B_UNIT> BS2B::BS2B_LOOKUP_TABLE()
	{
        InitializableQMap<BS2B_UNIT> table;
        table << QPair<BS2B_UNIT>(QObject::tr("BS2B Custom"), 99)
              << QPair<BS2B_UNIT>(QObject::tr("BS2B Weak"), 0)
              << QPair<BS2B_UNIT>(QObject::tr("BS2B Strong"), 1)
              << QPair<BS2B_UNIT>(QObject::tr("Out of head"), 2)
              << QPair<BS2B_UNIT>(QObject::tr("Surround 1"), 3)
              << QPair<BS2B_UNIT>(QObject::tr("Surround 2"), 4)
              << QPair<BS2B_UNIT>(QObject::tr("Joe0Bloggs Realistic surround"), 5);
        return table;
	}

	int BS2B::lookupPreset(const QString &preset)
	{
		auto table = BS2B_LOOKUP_TABLE();

		if (table.contains(preset))
		{
            for (const auto& [key, value] : table.toStdMap())
            {
                if (key == preset)
                {
                    return value;
                }
            }
		}

        return 0; // bs2b_weak
	}

	const QString BS2B::reverseLookup(int data)
	{
		auto table = BS2B_LOOKUP_TABLE();

        for (const auto& key : table.keys())
		{
			if (table[key] == data)
			{
            return key;
			}
		}

        return table.keys().first(); // fallback
	}

    const Reverb::sf_reverb_preset_data Reverb::lookupPreset(int preset)
	{
		sf_reverb_preset_data ps[] =
		{
			// OSF ERtoLt ERWet Dry ERFac ERWdth Wdth Wet Wander BassB Spin InpLP BasLP DmpLP OutLP RT60  Delay
			{ 1, 0.4, -9.0,  -7,  1.6,  0.7,   1.0,   -0,    0.25,    0.15,     0.7,      17000,      500,       7000,        10000,        3.2,        0.02                                           },
			{ 1, 0.3, -9.0,  -7,  1.0,  0.7,   1.0,   -8,    0.3,     0.25,     0.7,      18000,      600,       9000,        17000,        2.1,        0.01                                           },
			{ 1, 0.3, -9.0,  -7,  1.0,  0.7,   1.0,   -8,    0.25,    0.2,      0.5,      18000,      600,       7000,        9000,         2.3,        0.01                                           },
			{ 1, 0.3, -9.0,  -7,  1.2,  0.7,   1.0,   -8,    0.25,    0.2,      0.7,      18000,      500,       8000,        16000,        2.8,        0.01                                           },
			{ 1, 0.3, -9.0,  -7,  1.2,  0.7,   1.0,   -8,    0.2,     0.15,     0.5,      18000,      500,       6000,        8000,         2.9,        0.01                                           },
			{ 1, 0.2, -9.0,  -7,  1.4,  0.7,   1.0,   -8,    0.15,    0.2,      1.0,      18000,      400,       9000,        14000,        3.8,        0.018                                          },
			{ 1, 0.2, -9.0,  -7,  1.5,  0.7,   1.0,   -8,    0.2,     0.2,      0.5,      18000,      400,       5000,        7000,         4.2,        0.018                                          },
			{ 1, 0.7, -8.0,  -7,  0.7,  -0.4,  0.8,   -8,    0.2,     0.3,      1.6,      18000,      1000,      18000,       18000,        0.5,        0.005                                          },
			{ 1, 0.7, -8.0,  -7,  0.8,  0.6,   0.9,   -8,    0.3,     0.3,      0.4,      18000,      300,       10000,       18000,        0.5,        0.005                                          },
			{ 1, 0.5, -8.0,  -7,  1.2,  -0.4,  0.8,   -8,    0.2,     0.1,      1.6,      18000,      1000,      18000,       18000,        0.8,        0.008                                          },
			{ 1, 0.5, -8.0,  -7,  1.2,  0.6,   0.9,   -8,    0.3,     0.1,      0.4,      18000,      300,       10000,       18000,        1.2,        0.016                                          },
			{ 1, 0.2, -8.0,  -7,  2.2,  -0.4,  0.9,   -8,    0.2,     0.1,      1.6,      18000,      1000,      16000,       18000,        1.8,        0.01                                           },
			{ 1, 0.2, -8.0,  -7,  2.2,  0.6,   0.9,   -8,    0.3,     0.1,      0.4,      18000,      500,       9000,        18000,        1.9,        0.02                                           },
			{ 1, 0.5, -7.0,  -6,  1.2,  -0.4,  0.8,   -70,   0.2,     0.1,      1.6,      18000,      1000,      18000,       18000,        0.8,        0.008                                          },
			{ 1, 0.5, -7.0,  -6,  1.2,  0.6,   0.9,   -70,   0.3,     0.1,      0.4,      18000,      300,       10000,       18000,        1.2,        0.016                                          },
			{ 2, 0.0, -30.0, -12, 1.0,  1.0,   1.0,   -8,    0.2,     0.1,      1.6,      18000,      1000,      16000,       18000,        1.8,        0.0                                            },
			{ 2, 0.0, -30.0, -12, 1.0,  1.0,   1.0,   -8,    0.3,     0.2,      0.4,      18000,      500,       9000,        18000,        1.9,        0.0                                            },
			{ 2, 0.1, -16.0, -14, 1.0,  0.1,   1.0,   -5,    0.35,    0.05,     1.0,      18000,      100,       10000,       18000,        12.0,       0.0                                            },
			{ 2, 0.1, -16.0, -14, 1.0,  0.1,   1.0,   -5,    0.4,     0.05,     1.0,      18000,      100,       9000,        18000,        30.0,       0.0                                            }
		};
#define CASE(prs, i) \
	case prs: return ps[i];
		switch (preset)
		{
			CASE(SF_REVERB_PRESET_DEFAULT,      0)
			CASE(SF_REVERB_PRESET_SMALLHALL1,   1)
			CASE(SF_REVERB_PRESET_SMALLHALL2,   2)
			CASE(SF_REVERB_PRESET_MEDIUMHALL1,  3)
			CASE(SF_REVERB_PRESET_MEDIUMHALL2,  4)
			CASE(SF_REVERB_PRESET_LARGEHALL1,   5)
			CASE(SF_REVERB_PRESET_LARGEHALL2,   6)
			CASE(SF_REVERB_PRESET_SMALLROOM1,   7)
			CASE(SF_REVERB_PRESET_SMALLROOM2,   8)
			CASE(SF_REVERB_PRESET_MEDIUMROOM1,  9)
			CASE(SF_REVERB_PRESET_MEDIUMROOM2, 10)
			CASE(SF_REVERB_PRESET_LARGEROOM1,  11)
			CASE(SF_REVERB_PRESET_LARGEROOM2,  12)
			CASE(SF_REVERB_PRESET_MEDIUMER1,   13)
			CASE(SF_REVERB_PRESET_MEDIUMER2,   14)
			CASE(SF_REVERB_PRESET_PLATEHIGH,   15)
			CASE(SF_REVERB_PRESET_PLATELOW,    16)
			CASE(SF_REVERB_PRESET_LONGREVERB1, 17)
			CASE(SF_REVERB_PRESET_LONGREVERB2, 18)
		}

#undef CASE
		return ps[0];
	}

	const QStringList Reverb::getPresetNames()
    {
        return QStringList({ QObject::tr("Default"), QObject::tr("Small hall 1"), QObject::tr("Small hall 2"), QObject::tr("Medium hall 1"), QObject::tr("Medium hall 2"),
                             QObject::tr("Large hall 1"), QObject::tr("Large hall 2"), QObject::tr("Small room 1"), QObject::tr("Small room 2"), QObject::tr("Medium room 1"),
                             QObject::tr("Medium room 2"), QObject::tr("Large room 1"), QObject::tr("Large room 2"), QObject::tr("Medium ER 1"), QObject::tr("Medium ER 2"),
                             QObject::tr("Plate high"), QObject::tr("Plate low"), QObject::tr("Long reverb 1"), QObject::tr("Long reverb 2") });
	}

}
