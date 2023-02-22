#include "AppItem.h"
#include "ui_AppItem.h"

#include "data/model/AppItemModel.h"
#include "config/AppConfig.h"
#include "utils/Log.h"

AppItem::AppItem(AppItemModel* _model, int id, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AppItem),
    id(id)
{
    ui->setupUi(this);

    // Dummy mode
    if(id == -1)
    {
        return;
    }

    model = _model;

#ifdef USE_PULSEAUDIO
    ui->blocklist->hide();
#endif

    auto node = model->findByNodeId(id);
    if(!node.has_value())
    {
        Log::warning("AppItemModel::findByNodeId retuned nullopt");
        return;
    }

    refresh(node.value());

    ui->blocklist->setText(AppConfig::instance().get<bool>(AppConfig::AudioAppBlocklistInvert) ?
                           tr("Add to allowlist") : tr("Add to blocklist"));

    connect(&AppConfig::instance(), &AppConfig::updated, this, &AppItem::onAppConfigUpdated);
    connect(model, &AppItemModel::appChanged, this, &AppItem::refresh);
    connect(ui->blocklist, &QCheckBox::toggled, this, &AppItem::setBlocked);
}

AppItem::~AppItem()
{
    if(model != nullptr) // TODO: fix occasional sigsegv on exit
        disconnect(model, &AppItemModel::appChanged, this, &AppItem::refresh);
    disconnect(&AppConfig::instance(), &AppConfig::updated, this, &AppItem::onAppConfigUpdated);
    delete ui;
}

void AppItem::onAppConfigUpdated(const AppConfig::Key& key, const QVariant& value)
{
    switch (key) {
    case AppConfig::AudioAppBlocklistInvert: {
        ui->blocklist->setText(value.toBool() ? tr("Add to allowlist") : tr("Add to blocklist"));
        auto node = model->findByNodeId(id);
        if(!node.has_value())
        {
            Log::warning("AppItemModel::findByNodeId retuned nullopt");
            return;
        }

        refresh(node.value());
        break;
    }
    default:
        break;
    }
}

void AppItem::refresh(const AppNode& node)
{
    if(node.id != id)
    {
        return;
    }

    ui->name->setText(node.name);
    ui->rate->setText(tr("Rate: %1Hz").arg(QString::number(node.rate)));
    ui->latency->setText(tr("Latency: %1ms").arg(QString::number(node.latency * 1000, 'f', 2)));
    ui->format->setText(tr("Format: %1").arg(node.format));
    ui->state->setText(node.state);
    ui->icon->setPixmap(QIcon::fromTheme(node.app_icon_name, QIcon(":/icons/Procedure_16x.svg")).pixmap(QSize(16, 16)));

    auto list = AppConfig::instance().get<QStringList>(AppConfig::AudioAppBlocklist);
    ui->blocklist->setChecked(list.contains(node.name));
}

void AppItem::setBlocked(bool blocked)
{
    auto node = model->findByNodeId(id);
    if(!node.has_value())
    {
        Log::error("AppItemModel::findByNodeId retuned nullopt");
        return;
    }

    Log::debug(node.value().name + " has been " + (blocked ? "blocked" : "unblocked"));

    auto list = AppConfig::instance().get<QStringList>(AppConfig::AudioAppBlocklist);
    if(blocked)
    {
        if(!list.contains(node.value().name))
        {
            list.append(node.value().name);
        }
        else
        {
            Log::debug("Already in list, ignoring");
        }
    }
    else if(list.contains(node.value().name))
    {
        list.removeAll(node.value().name);
    }
    else
    {
        Log::debug("Not in list, ignoring");
    }

    AppConfig::instance().set(AppConfig::AudioAppBlocklist, list);
}
