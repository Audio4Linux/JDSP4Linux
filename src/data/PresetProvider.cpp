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
		return EQ_LOOKUP_TABLE()["Default"];
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
			return table["Default"];
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

		return "Custom";
	}

	const QMap<EQ_UNIT> EQ::EQ_LOOKUP_TABLE()
	{
		InitializableQMap<EQ_UNIT> table;
		table << QPair<EQ_UNIT>("Default", DOUBLE_LIST({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }))
		      << QPair<EQ_UNIT>("Pop", DOUBLE_LIST({ 0.0, 0.0, 0.0, 0.0, 0.0, 1.3, 2.0, 2.5, 5.0, -1.5, -2.0, -3.0, -3.0, -3.0, -3.0 }))
		      << QPair<EQ_UNIT>("Rock", DOUBLE_LIST({ 0.0, 0.0, 0.0, 3.0, 3.0, -10.0, -4.0, -1.0, 0.8, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0 }))
		      << QPair<EQ_UNIT>("Jazz", DOUBLE_LIST({ 0.0, 0.0, 0.0, 2.0, 4.0, 5.9, -5.9, -4.5, -2.5, 2.5, 1.0, -0.8, -0.8, -0.8, -0.8 }))
		      << QPair<EQ_UNIT>("Classic", DOUBLE_LIST({ -0.3, 0.3, -3.5, -9.0, -1.0, 0.0, 1.8, 2.1, 0.0, 0.0, 0.0, 4.4, 9.0, 9.0, 9.0 }))
		      << QPair<EQ_UNIT>("Bass", DOUBLE_LIST({ 10.00, 8.80, 8.50, 6.50, 2.50, 1.50, 0, 0, 0, 0, 0, 0, 0, 0, 0 }))
		      << QPair<EQ_UNIT>("Clear", DOUBLE_LIST({ 3.5, 5.5, 6.5, 9.5, 8.0, 6.5, 3.5, 2.5, 1.3, 5.0, 7.0, 9.0, 10.0, 11.0, 9.0 }))
		      << QPair<EQ_UNIT>("Volume Boost", DOUBLE_LIST({ 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, }))
		      << QPair<EQ_UNIT>("Hip-Hop", DOUBLE_LIST({ 4.5, 4.3, 4.0, 2.5, 1.5, 3.0, -1.0, -1.5, -1.5, 1.5, 0.0, -1.0, 0.0, 1.5, 3.0 }))
		      << QPair<EQ_UNIT>("Dubstep", DOUBLE_LIST({ 12.0, 10.0, 0.5, -1.0, -3.0, -5.0, -5.0, -4.8, -4.5, -2.5, -1.0, 0.0, -2.5, -2.5, 0.0 }))
		      << QPair<EQ_UNIT>("Movie", DOUBLE_LIST({ 3.0, 3.0, 6.1, 8.5, 9.0, 7.0, 6.1, 6.1, 5.0, 8.0, 3.5, 3.5, 8.0, 10.0, 8.0 }))
		      << QPair<EQ_UNIT>("Metal", DOUBLE_LIST({ 10.5, 10.5, 7.5, 0.0, 2.0, 5.5, 0.0, 0.0, 0.0, 6.1, 0.0, 0.0, 6.1, 10.0, 12.0 }))
		      << QPair<EQ_UNIT>("Vocal Booster", DOUBLE_LIST({ -1.5, -2.0, -3.0, -3.0, -0.5, 1.5, 3.5, 3.5, 3.5, 3.0, 2.0, 1.5, 0.0, 0.0, -1.5 }))
		      << QPair<EQ_UNIT>("Hardstyle", DOUBLE_LIST({ 6.1, 7.0, 12.0, 6.1, -5.0, -12.0, -2.5, 3.0, 6.5, 0.0, -2.2, -4.5, -6.1, -9.2, -10.0 }))
		      << QPair<EQ_UNIT>("Acoustic", DOUBLE_LIST({ 5.00, 4.50, 4.00, 3.50, 1.50, 1.00, 1.50, 1.50, 2.00, 3.00, 3.50, 4.00, 3.70, 3.00, 3.00 }))
		      << QPair<EQ_UNIT>("R&B", DOUBLE_LIST({ 3.0, 3.0, 7.0, 6.1, 4.5, 1.5, -1.5, -2.0, -1.5, 2.0, 2.5, 3.0, 3.5, 3.8, 4.0 }))
		      << QPair<EQ_UNIT>("Electronic", DOUBLE_LIST({ 4.0, 4.0, 3.5, 1.0, 0.0, -0.5, -2.0, 0.0, 2.0, 0.0, 0.0, 1.0, 3.0, 4.0, 4.5 }))
		      << QPair<EQ_UNIT>("Deep Bass", DOUBLE_LIST({ 12.0, 8.0, 0.0, -6.7, -12.0, -9.0, -3.5, -3.5, -6.1, 0.0, -3.0, -5.0, 0.0, 1.2, 3.0 }))
		      << QPair<EQ_UNIT>("Beats", DOUBLE_LIST({ -5.5, -5.0, -4.5, -4.2, -3.5, -3.0, -1.9, 0, 0, 0, 0, 0, 0, 0, 0 }))
		      << QPair<EQ_UNIT>("Soft Bass", DOUBLE_LIST({ 12.0, 11.0, 10.3, 9.5, 8.0, 7.0, 6.3, 5.5, 5.0, 6.1, 6.1, 3.5, 10.0, 10.5, 8.0 }));
		return std::move(table);
	}

	const QMap<BS2B_UNIT> BS2B::BS2B_LOOKUP_TABLE()
	{
		InitializableQMap<BS2B_UNIT> table;
		table << QPair<BS2B_UNIT>("BS2B Custom", 6)
		      << QPair<BS2B_UNIT>("BS2B Weak", 0)
		      << QPair<BS2B_UNIT>("BS2B Strong", 1)
		      << QPair<BS2B_UNIT>("Out of head", 2)
		      << QPair<BS2B_UNIT>("Surround 1", 3)
		      << QPair<BS2B_UNIT>("Surround 2", 4)
		      << QPair<BS2B_UNIT>("Joe0Bloggs Realistic surround", 5);
		return std::move(table);
	}

	int BS2B::lookupPreset(const QString &preset)
	{
		auto table = BS2B_LOOKUP_TABLE();

		if (table.contains(preset))
		{
			return table[preset];
		}
		else
		{
			return table["BS2B Weak"];
		}
	}

	const QString BS2B::reverseLookup(int data)
	{
		auto table = BS2B_LOOKUP_TABLE();

		for (auto key : table.keys())
		{
			if (table[key] == data)
			{
				return key;
			}
		}

		return "BS2B Weak";
	}

	const QMap<SW_UNIT> StereoWidener::SW_LOOKUP_TABLE()
	{
		InitializableQMap<SW_UNIT> table;
		table << QPair<SW_UNIT>("Unknown", FLOAT_LIST({ 0, 0 }))
		      << QPair<SW_UNIT>("A Bit", FLOAT_LIST({ 1.0 * 0.5, 1.2 * 0.5 }))
		      << QPair<SW_UNIT>("Slight", FLOAT_LIST({ 0.95 * 0.5, 1.4 * 0.5 }))
		      << QPair<SW_UNIT>("Moderate", FLOAT_LIST({ 0.9 * 0.5, 1.6 * 0.5 }))
		      << QPair<SW_UNIT>("High", FLOAT_LIST({ 0.85 * 0.5, 1.8 * 0.5 }))
		      << QPair<SW_UNIT>("Super", FLOAT_LIST({ 0.8 * 0.5, 2.0 * 0.5 }));
		return std::move(table);
	}

	const FLOAT_LIST StereoWidener::lookupPreset(const QString &preset)
	{
		auto table = SW_LOOKUP_TABLE();

		if (table.contains(preset))
		{
			return table[preset];
		}
		else
		{
			return table["Unknown"];
		}
	}

	const QString StereoWidener::reverseLookup(const QVector<float> &data)
	{
		auto table = SW_LOOKUP_TABLE();

		for (auto key : table.keys())
		{
			QVector<float> row(table[key]);
			int            it        = 0;
			bool           different = false;

			for (auto cur_data : row)
			{
				bool equal = isApproximatelyEqual<float>(cur_data, data.at(it), 0.01);

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

		return "...";
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
		return QStringList({ "Default", "Small hall 1", "Small hall 2", "Medium hall 1", "Medium hall 2",
		                     "Large hall 1", "Large hall 2", "Small room 1", "Small room 2", "Medium room 1",
		                     "Medium room 2", "Large room 1", "Large room 2", "Medium ER 1", "Medium ER 2",
		                     "Plate high", "Plate low", "Long reverb 1", "Long reverb 2" });
	}

}