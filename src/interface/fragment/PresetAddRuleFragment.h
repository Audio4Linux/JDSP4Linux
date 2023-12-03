#ifndef PRESETADDRULEFRAGMENT_H
#define PRESETADDRULEFRAGMENT_H

#include "data/PresetRule.h"
#include "interface/fragment/BaseFragment.h"

class DeviceListModel;
class PresetListModel;
class RouteListModel;
class PresetRuleTableModel;

namespace Ui {
class PresetAddRuleFragment;
}

class PresetAddRuleFragment : public BaseFragment
{
    Q_OBJECT

public:
    explicit PresetAddRuleFragment(DeviceListModel* deviceModel, PresetListModel* presetModel, PresetRuleTableModel* tableModel, QWidget *parent = nullptr);
    ~PresetAddRuleFragment();

    PresetRule rule() const;

protected:
    void showEvent(QShowEvent* ev) override;

signals:
    void accepted();

private slots:
    void onDeviceChanged();

private:
    Ui::PresetAddRuleFragment *ui;

    RouteListModel* routeModel;
    PresetRuleTableModel* tableModel;

};

#endif // PRESETADDRULEFRAGMENT_H
