#ifndef PRESETRULEDIALOG_H
#define PRESETRULEDIALOG_H

#include <QDialog>
#include <QItemSelection>

class DeviceListModel;
class PresetListModel;
class PresetRuleTableModel;
class PresetRuleTableDelegate;
class PresetAddRuleFragment;
class IAudioService;

template<class T>
class FragmentHost;

namespace Ui {
class PresetRuleDialog;
}

class PresetRuleDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PresetRuleDialog(IAudioService* service, QWidget *parent = nullptr);
    ~PresetRuleDialog();

protected:
    void showEvent(QShowEvent* event);

private slots:
    void onAddClicked();
    void onAddConfirmed();
    void onRemoveClicked();
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
    Ui::PresetRuleDialog *ui;

    IAudioService* service;
    DeviceListModel* deviceModel;
    PresetListModel* presetModel;
    PresetRuleTableModel* ruleModel;

    PresetRuleTableDelegate* ruleDelegate;

    FragmentHost<PresetAddRuleFragment*>* addRuleFragment;
};

#endif // PRESETRULEDIALOG_H
