#include "PresetAddRuleFragment.h"
#include "ui_PresetAddRuleFragment.h"

#include "data/model/DeviceListModel.h"
#include "data/model/PresetListModel.h"

#include <QAbstractButton>

PresetAddRuleFragment::PresetAddRuleFragment(DeviceListModel *deviceModel, PresetListModel *presetModel, QWidget *parent) :
    BaseFragment(parent),
    ui(new Ui::PresetAddRuleFragment)
{
    ui->setupUi(this);

    ui->comboIf->setModel(deviceModel);
    ui->comboThen->setModel(presetModel);

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &PresetAddRuleFragment::closePressed);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &PresetAddRuleFragment::closePressed);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &PresetAddRuleFragment::accepted);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this]{
        ui->buttonBox->setDisabled(true);
    });
}

PresetAddRuleFragment::~PresetAddRuleFragment()
{
    delete ui;
}

PresetRule PresetAddRuleFragment::rule() const
{
    return PresetRule(ui->comboIf->currentData().value<IOutputDevice>(),
                      "*", // TODO
                      ui->comboThen->currentData().toString());
}

void PresetAddRuleFragment::showEvent(QShowEvent *ev)
{
    ui->buttonBox->setDisabled(false);
    BaseFragment::showEvent(ev);
}
