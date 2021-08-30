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
#ifndef PRESET_H
#define PRESET_H

#include <QDialog>
#include <QNetworkReply>

class AppConfig;
class MainWindow;

namespace Ui
{
    class PresetDialog;
}

class PresetDialog :
	public QDialog
{
	Q_OBJECT

public:
	explicit PresetDialog(QWidget *parent = nullptr);
	void UpdateList();

	~PresetDialog();

private slots:
	void reject();
	void add();
	void remove();
	void load();
	void importLinux();
	void exportLinux();
	void nameChanged(const QString &);
	void showContextMenu(const QPoint &pos);
	void presetIndexChanged();

private:
    Ui::PresetDialog *ui;
	QStringList irs;

signals:
	void presetChanged();

};

#endif // PRESET_H
