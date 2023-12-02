#include "PresetRuleDialog.h"
#include "ui_PresetRuleDialog.h"

#include "data/PresetManager.h"
#include "data/PresetRuleTableDelegate.h"
#include "data/model/DeviceListModel.h"
#include "data/model/PresetListModel.h"
#include "data/model/PresetRuleTableModel.h"
#include "interface/fragment/FragmentHost.h"
#include "interface/fragment/PresetAddRuleFragment.h"

#include "IAudioService.h"

#include <QMessageBox>

PresetRuleDialog::PresetRuleDialog(IAudioService* service, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PresetRuleDialog),
    service(service),
    deviceModel(new DeviceListModel(service, this)),
    presetModel(PresetManager::instance().presetModel()),
    ruleModel(new PresetRuleTableModel(this)),
    ruleDelegate(new PresetRuleTableDelegate(this)),
    addRuleFragment(new FragmentHost<PresetAddRuleFragment*>(new PresetAddRuleFragment(deviceModel, presetModel, this),
                                                             WAF::BottomSide, this))
{
    ui->setupUi(this);

    ruleDelegate->attachModels(deviceModel, presetModel, ruleModel);
    ui->ruleTable->setItemDelegate(ruleDelegate);
    ui->ruleTable->setModel(ruleModel);
    ui->ruleTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->ruleTable->setEmptyViewEnabled(true);
    ui->ruleTable->setEmptyViewTitle(tr("No rules defined"));

    ruleModel->load();
    deviceModel->load(service->outputDevices());
    presetModel->rescan();

    ui->remove->setEnabled(false);

    connect(ui->ruleTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PresetRuleDialog::onSelectionChanged);

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &PresetRuleDialog::reject);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, ruleModel, &PresetRuleTableModel::commit);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &PresetRuleDialog::accept);

    connect(ui->remove, &QPushButton::clicked, this, &PresetRuleDialog::onRemoveClicked);
    connect(ui->add, &QPushButton::clicked, this, &PresetRuleDialog::onAddClicked);
    connect(addRuleFragment->fragment(), &PresetAddRuleFragment::accepted, this, &PresetRuleDialog::onAddConfirmed);
}

PresetRuleDialog::~PresetRuleDialog()
{
    delete ui;
}

void PresetRuleDialog::onAddClicked()
{
    if(!deviceModel->loadRemaining(ruleModel))
    {
        QMessageBox::information(this, tr("Cannot add new rule"),
                                 tr("All connected audio devices have already a rule defined.\n"
                                 "You can only create one rule per device."));
        return;
    }

    if(PresetManager::instance().presetModel()->rowCount() <= 0)
    {
        QMessageBox::information(this, tr("Cannot add new rule"), tr("You have no presets saved.\n"
                                                              "Please create one first before adding a new rule."));
        return;
    }

    presetModel->rescan();
    addRuleFragment->slideIn();
}

void PresetRuleDialog::onAddConfirmed()
{
    ruleModel->add(addRuleFragment->fragment()->rule());
}

void PresetRuleDialog::onRemoveClicked()
{
    auto* select = ui->ruleTable->selectionModel();
    if (select->hasSelection())
    {
        auto indexes = select->selectedRows();
        for(int i = indexes.count(); i > 0; i--)
        {
            ruleModel->removeRow(indexes.at(i-1).row(), QModelIndex());
        }
    }
}

void PresetRuleDialog::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)

    ui->remove->setEnabled(!selected.empty());
}
