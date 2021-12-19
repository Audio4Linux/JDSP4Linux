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
#include <QItemSelection>
#include <QMenu>

#include "interface/fragment/BaseFragment.h"

class AppConfig;
class PresetRuleDialog;
class IAudioService;

namespace Ui
{
    class PresetDialog;
}

class PresetFragment :
    public BaseFragment
{
	Q_OBJECT

public:
    explicit PresetFragment(IAudioService* service, QWidget *parent = nullptr);
    ~PresetFragment();

    void updateList();

private slots:
    void onAddClicked();
    void onRemoveClicked();
    void onLoadClicked();
    void onNameFieldChanged(const QString &);
    void onContextMenuRequested(const QPoint &pos);
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
    Ui::PresetDialog *ui;
    PresetRuleDialog *ruleDialog;
    QMenu ctxMenu;

};

#endif // PRESET_H
