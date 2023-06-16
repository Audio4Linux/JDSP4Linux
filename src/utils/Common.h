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
#ifndef COMMON_H
#define COMMON_H

#include <QDir>
#include <QMenu>

#include <cmath>

template<typename TReal>
static bool isApproximatelyEqual(TReal a,
                                 TReal b,
                                 TReal tolerance = std::numeric_limits<TReal>::epsilon())
{
    TReal diff = std::fabs(a - b);

    if (diff <= tolerance)
    {
        return true;
    }

    if (diff < std::fmax(std::fabs(a), std::fabs(b)) * tolerance)
    {
        return true;
    }

    return false;
}

static inline QString pathAppend(const QString &path1,
                                 const QString &path2)
{
	return QDir::cleanPath(path1 + QDir::separator() + path2);
}

static QString chopDoubleQuotes(QString str)
{
	if (str.size() > 2)
	{
		if (str.at(0) == '"')
		{
			str.remove(0, 1);                // remove double quotes
		}

		if (str.at(str.length() - 1) == '"')
		{
			str.chop(1);
		}

		return str;
	}

	return "";
}

namespace MenuIO
{
	static QString buildString(QMenu *menu)
	{
		QString out;

		for (auto action : menu->actions())
		{
			if (action->isSeparator())
			{
				out += "separator;";
			}
			else if (action->menu())
			{
				out += action->menu()->property("tag").toString() + ";";
			}
			else
			{
				out += action->property("tag").toString() + ";";
			}
		}

		return out;
	}

	static QMenu* buildMenu(QMenu   *options,
	                        QString  input,
	                        QWidget *owner)
	{
		QMenu *out = new QMenu(owner);

		for (auto item : input.split(";"))
		{
			if (item == "separator")
			{
				out->addSeparator();
			}
			else if (item.startsWith("menu"))
			{
				for (auto action : options->actions())
				{
					if (action->menu())
					{
						if (action->menu()->property("tag") == item)
						{
							out->addMenu(action->menu());
							continue;
						}
					}
				}
			}
			else
			{
				for (auto action : options->actions())
				{
					if (action->property("tag") == item)
					{
						out->addAction(action);
						continue;
					}
				}
			}
		}

		return out;
	}

}
#endif // COMMON_H
