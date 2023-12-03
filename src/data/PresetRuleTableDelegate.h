#ifndef PRESETRULETABLEDELEGATE_H
#define PRESETRULETABLEDELEGATE_H

#include <QComboBox>
#include <QMessageBox>
#include <QStyledItemDelegate>

#include "model/DeviceListModel.h"
#include "model/PresetListModel.h"
#include "model/PresetRuleTableModel.h"
#include "utils/Log.h"

class PresetRuleTableDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    PresetRuleTableDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void attachModels(DeviceListModel* devModel,
                      PresetListModel* presetModel,
                      PresetRuleTableModel* ruleModel)
    {
        _deviceModel = devModel;
        _presetModel = presetModel;
        _ruleModel = ruleModel;
    };

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const Q_DECL_OVERRIDE
    {
        Q_UNUSED(option)

        QComboBox *cb = new QComboBox(parent);
        switch(index.column())
        {
        case 2:
            cb->setModel(_presetModel);
            break;
        }
        return cb;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const Q_DECL_OVERRIDE
    {
        if(!_ruleModel)
        {
            Log::error("Rule model not attached");
            return;
        }

        auto rule = _ruleModel->at(index);
        const QString currentText = index.data(Qt::EditRole).toString();

        QComboBox *cb = qobject_cast<QComboBox *>(editor);
        if(cb)
        {
            const int cbIndex = cb->findText(currentText);
            if (cbIndex >= 0)
                cb->setCurrentIndex(cbIndex);
        }
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const Q_DECL_OVERRIDE
    {
        QComboBox *cb = qobject_cast<QComboBox *>(editor);
        if(cb)
        {
            model->setData(index, cb->currentText(), Qt::EditRole);
        }
    }

private:
    DeviceListModel* _deviceModel = nullptr;
    PresetRuleTableModel* _ruleModel = nullptr;
    PresetListModel* _presetModel = nullptr;

};

#endif // PRESETRULETABLEDELEGATE_H
