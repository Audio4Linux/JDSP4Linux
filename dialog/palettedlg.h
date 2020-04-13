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
#ifndef PALETTE_H
#define PALETTE_H

#include <QDialog>
#include <config/appconfigwrapper.h>

namespace Ui {
class palette;
}

class PaletteEditor : public QDialog
{
    Q_OBJECT

public:
    explicit PaletteEditor(AppConfigWrapper *_appconf, QWidget *parent = nullptr);
    ~PaletteEditor();

private:
    Ui::palette *ui;
    AppConfigWrapper *appconf;
private slots:
    int loadColor(int index,int rgb_index);
    void closeWin();
    void Reset();
    void updateBase();
    void updateBack();
    void updateFore();
    void updateIcons();
    void updateSelection();
    void updateDisabled();
    void saveColor(int index,const QColor& color);
};

#endif // PALETTE_H
