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
#ifndef STATUSDIALOG_H
#define STATUSDIALOG_H

#include <QDialog>

namespace Ui
{
	class StatusDialog;
}

class StatusFragment :
	public QDialog
{
	Q_OBJECT

public:
	explicit StatusFragment(QWidget *parent = nullptr);
	~StatusFragment();

signals:
	void closePressed();

private:
	Ui::StatusDialog *ui;
};

#endif // STATUSDIALOG_H