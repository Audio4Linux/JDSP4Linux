#include "PresetFragment.h"
#include "ui_PresetFragment.h"

#include "config/AppConfig.h"
#include "data/PresetManager.h"
#include "data/model/PresetListModel.h"
#include "interface/dialog/PresetRuleDialog.h"
#include "utils/Log.h"

#include <QFile>
#include <QInputDialog>
#include <QMessageBox>

PresetFragment::PresetFragment(IAudioService* service, QWidget *parent) :
    BaseFragment(parent),
    ui(new Ui::PresetDialog),
    ruleDialog(new PresetRuleDialog(service, this))
{
	ui->setupUi(this);

    ui->files->setModel(PresetManager::instance().presetModel());
    ui->files->setEmptyViewEnabled(true);
    ui->files->setEmptyViewTitle(tr("No presets saved"));

    ui->add->setEnabled(false);
    ui->remove->setEnabled(false);
    ui->load->setEnabled(false);

#ifdef USE_PULSEAUDIO
    ui->rules->setVisible(false);
#endif

    connect(ui->add, &QPushButton::clicked, this, &PresetFragment::onAddClicked);
    connect(ui->load, &QPushButton::clicked, this, &PresetFragment::onLoadClicked);
    connect(ui->remove, &QPushButton::clicked, this, &PresetFragment::onRemoveClicked);
    connect(ui->rules, &QPushButton::clicked, ruleDialog, &PresetRuleDialog::exec);

    connect(ui->presetName, &QLineEdit::textChanged, this, &PresetFragment::onNameFieldChanged);

    connect(ui->files, &CListView::customContextMenuRequested, this, &PresetFragment::onContextMenuRequested);
    connect(ui->files->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PresetFragment::onSelectionChanged);
}

PresetFragment::~PresetFragment()
{
	delete ui;
}

void PresetFragment::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)

    ui->load->setEnabled(!selected.empty());
    ui->remove->setEnabled(!selected.empty());

    if (selected.empty() || selected.indexes().empty())
	{
		return;
	}

    ui->presetName->setText(PresetManager::instance().presetModel()->data(selected.indexes().first(), Qt::UserRole).toString());
}

void PresetFragment::onAddClicked()
{
    if(ui->presetName->text().isEmpty())
    {
        return;
    }

    PresetManager::instance().save(ui->presetName->text());
	ui->presetName->text() = "";
}

void PresetFragment::onRemoveClicked()
{
    auto* select = ui->files->selectionModel();
    if (!select->hasSelection())
    {
        return;
    }

    auto indexes = select->selectedIndexes();
    PresetManager::instance().remove(PresetManager::instance().presetModel()->data(indexes.first(), Qt::UserRole).toString());

    ui->load->setEnabled(false);
    ui->remove->setEnabled(false);
}

void PresetFragment::onLoadClicked()
{
    auto* select = ui->files->selectionModel();
    if (!select->hasSelection())
    {
        return;
    }

    auto indexes = select->selectedIndexes();
    if(!PresetManager::instance().load(PresetManager::instance().presetModel()->data(indexes.first(), Qt::UserRole).toString()))
    {
        QMessageBox::warning(this, tr("Cannot load preset"), tr("Selected file does not exist anymore"), QMessageBox::Ok);
    }
}

void PresetFragment::onNameFieldChanged(const QString &name)
{
    ui->add->setEnabled(!name.isEmpty());

    if (PresetManager::instance().exists(name))
	{
		ui->add->setText(tr("Overwrite"));
	}
	else
	{
		ui->add->setText(tr("Save"));
	}
}

void PresetFragment::onContextMenuRequested(const QPoint &pos)
{
    auto globalPos = ui->files->mapToGlobal(pos);
    auto actionRename = ctxMenu.addAction(tr("Rename"));
    auto actionDelete = ctxMenu.addAction(tr("Delete"));
    auto preset = PresetManager::instance().presetModel()->data(ui->files->indexAt(pos), Qt::UserRole);

    if (!preset.isValid() || preset.isNull())
	{
		return;
	}

    auto result = ctxMenu.exec(globalPos);
    if (!result)
    {
        return;
    }

    if (result == actionRename)
    {
        bool ok;
        QString newName = QInputDialog::getText(this, tr("Rename preset"),
                                             tr("Enter new name"), QLineEdit::Normal,
                                             preset.toString(), &ok);

        if (ok && !newName.isEmpty())
        {
            PresetManager::instance().rename(preset.toString(), newName);
        }
    }
    else if (result == actionDelete)
    {
        PresetManager::instance().remove(preset.toString());
    }
}
