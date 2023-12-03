#include "PresetAddRuleFragment.h"
#include "ui_PresetAddRuleFragment.h"

#include "data/model/DeviceListModel.h"
#include "data/model/PresetListModel.h"
#include "data/model/RouteListModel.h"

#include <QAbstractButton>

PresetAddRuleFragment::PresetAddRuleFragment(DeviceListModel *deviceModel, PresetListModel *presetModel, PresetRuleTableModel* tableModel, QWidget *parent) :
    BaseFragment(parent),
    ui(new Ui::PresetAddRuleFragment),
    routeModel(new RouteListModel()),
    tableModel(tableModel)
{
    ui->setupUi(this);

    ui->comboIf->setModel(deviceModel);
    ui->comboRoute->setModel(routeModel);
    ui->comboThen->setModel(presetModel);

    connect(ui->comboIf, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PresetAddRuleFragment::onDeviceChanged);

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
    delete routeModel;
}

PresetRule PresetAddRuleFragment::rule() const
{
    return PresetRule(ui->comboIf->currentData().value<IOutputDevice>(),
                      ui->comboRoute->currentData().value<Route>(),
                      ui->comboThen->currentData().toString());
}

void PresetAddRuleFragment::showEvent(QShowEvent *ev)
{
    ui->buttonBox->setDisabled(false);
    BaseFragment::showEvent(ev);
}

void PresetAddRuleFragment::onDeviceChanged()
{
    routeModel->loadRemaining(ui->comboIf->currentData().value<IOutputDevice>(), tableModel);
}
