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
#ifndef LOG_H
#define LOG_H

#include <QDialog>

namespace Ui {
class log;
}

class LogDlg : public QDialog
{
    Q_OBJECT

public:
    explicit LogDlg(QWidget *parent = nullptr);
    ~LogDlg();
public slots:
    void updateLog();
private slots:
    void reject();
private:
    Ui::log *ui;
};

#endif // LOG_H
