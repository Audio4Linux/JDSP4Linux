#ifndef PRESETADDRULEFRAGMENT_H
#define PRESETADDRULEFRAGMENT_H

#include "data/PresetRule.h"
#include "interface/fragment/BaseFragment.h"

class DeviceListModel;
class PresetListModel;

namespace Ui {
class PresetAddRuleFragment;
}

class PresetAddRuleFragment : public BaseFragment
{
    Q_OBJECT

public:
    explicit PresetAddRuleFragment(DeviceListModel* deviceModel, PresetListModel* presetModel, QWidget *parent = nullptr);
    ~PresetAddRuleFragment();

    PresetRule rule() const;

protected:
    void showEvent(QShowEvent* ev) override;

signals:
    void accepted();

private:
    Ui::PresetAddRuleFragment *ui;

};

#endif // PRESETADDRULEFRAGMENT_H
