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
#ifndef MATHFUNCTIONS_H
#define MATHFUNCTIONS_H
#include <cmath>
#include <QString>

class MathFunctions
{
private:
	static QString buildEqGainString(int f)
	{
		QString pre("");

		if (f < 0 )
		{
			pre = "-";
		}

		QString s;

		if (QString::number(abs(f) % 100).length() == 1)
		{
			char buffer[5];
			snprintf(buffer, sizeof(buffer), "%02d", abs(f) % 100);
			s = pre + QString::number(abs(f) / 100) + "."  + QString::fromUtf8(buffer) + "dB";
		}
		else
		{
			s = pre + QString::number(abs(f) / 100) + "."  + QString::number(abs(f % 100)) + "dB";
		}

		return s;
	}

};

#endif // MATHFUNCTIONS_H