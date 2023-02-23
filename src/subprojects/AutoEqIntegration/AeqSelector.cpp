#include "AeqPackageManager.h"
#include "AeqSelector.h"
#include "HttpException.h"
#include "ui_AeqSelector.h"

#include "AeqListDelegates.h"
#include "AeqMeasurementItem.h"
#include "AeqMeasurementModel.h"

#include "config/AppConfig.h"

#include <QListWidget>
#include <QMessageBox>
#include <QBitmap>
#include <QSortFilterProxyModel>

AeqSelector::AeqSelector(QWidget *parent) :
	QDialog(parent),
    ui(new Ui::AeqSelector),
    pkgManager(new AeqPackageManager(this)),
    model(new AeqMeasurementModel(this)),
    proxyModel(new QSortFilterProxyModel(this))
{
    ui->setupUi(this);

	ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(0);
    ui->previewStack->setCurrentIndex(0);

    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setDynamicSortFilter(false);
    proxyModel->setSourceModel(model);

    ui->list->setEmptyViewEnabled(true);
    ui->list->setEmptyViewTitle(tr("No measurements found"));
    ui->list->setModel(proxyModel);
    ui->list->setItemDelegate(new AeqItemDelegate(ui->list));

    ui->darkMode->setChecked(AppConfig::instance().get<bool>(AppConfig::AeqPlotDarkMode));

    connect(ui->buttonBox, &QDialogButtonBox::accepted,   this, &AeqSelector::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,   this, &AeqSelector::reject);

    connect(ui->searchInput,    &QLineEdit::textChanged,  proxyModel, &QSortFilterProxyModel::setFilterWildcard);
    connect(ui->manageDatabase, &QPushButton::clicked,    this,        &AeqSelector::switchPane);
    connect(ui->updateButton,   &QPushButton::clicked,    this,        &AeqSelector::updateDatabase);
    connect(ui->deleteButton,   &QPushButton::clicked,    this,        &AeqSelector::deleteDatabase);

    connect(ui->darkMode,       &QCheckBox::stateChanged, this,        &AeqSelector::onDarkModeToggled);

    connect(ui->list->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AeqSelector::onSelectionChanged);

    updateDatabaseInfo();
}

AeqSelector::~AeqSelector()
{
    delete ui;
}

void AeqSelector::forceManageMode()
{
    ui->manageDatabase->setVisible(false);
    ui->stackedWidget->setCurrentIndex(1);
    ui->buttonBox->clear();
    ui->buttonBox->addButton(QDialogButtonBox::Close);
    ui->frame->setVisible(false);

    auto geo = this->geometry();
    geo.setWidth(geo.width() / 2);
    this->setGeometry(geo);
}

void AeqSelector::showEvent(QShowEvent *ev)
{   
    if(!pkgManager->isPackageInstalled())
    {
        this->setEnabled(false);
        auto parent = this->parent() == nullptr ? this : this->parentWidget();
        auto res = QMessageBox::question(parent, tr("AutoEQ database"),
                      tr("Before using the AutoEQ integration, you need to download a minified version of their headphone compensation database (~50MB) to your hard drive.\n"
                      "An internet connection is required during this step.\n\n"
                      "Do you want to continue and enable this feature?"),
                      QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);

        auto cancel = [this]{ QTimer::singleShot(250, this, &QDialog::reject); };
        if(res == QMessageBox::Cancel)
        {
            cancel();
            return;
        }

        pkgManager->getRepositoryVersion().then([this, cancel](AeqVersion remote)
        {
            pkgManager->installPackage(remote, this).then([this]()
            {
                this->setEnabled(true);
                updateDatabaseInfo();
            }).fail([cancel](){
                cancel();
                return;
            });

        }).fail([this, cancel](const HttpException& ex){
            QMessageBox::critical(this, tr("Failed to retrieve version information"), tr("Failed to retrieve package information from the remote repository:\n\n"
                                                                                          "Status code: %0\nReason: %1").arg(ex.statusCode()).arg(ex.reasonPhrase()));
            cancel();
            return;
        });
    }

    QDialog::showEvent(ev);
}

void AeqSelector::switchPane()
{
    ui->manageDatabase->setText(!ui->stackedWidget->currentIndex() ? tr("Return to database") : tr("Manage database"));
    ui->stackedWidget->setCurrentIndex(!ui->stackedWidget->currentIndex());
}

void AeqSelector::updateDatabase()
{
    this->setEnabled(false);

    auto doInstallation = [this](AeqVersion remote){
        pkgManager->installPackage(remote, this).then([this]()
        {
            updateDatabaseInfo();
        });
    };

    pkgManager->isUpdateAvailable().then([doInstallation](AeqVersion remote)
    {
        doInstallation(remote);
    }).fail([this](const HttpException& ex){
        QMessageBox::critical(this, tr("Failed to retrieve version information"), tr("Failed to retrieve package information from the remote repository:\n\n"
                                                                                     "Status code: %0\nReason: %1").arg(ex.statusCode()).arg(ex.reasonPhrase()));
    }).fail([this, doInstallation](const AeqVersion& remote){
        auto button = QMessageBox::question(this, tr("No new updates available"),
                                            tr("The local database is currently up-to-date; no new updates are available at this time.\n\n"
                                               "It may take up to 24 hours for new changes in the AutoEQ upstream repo to become available for download here. "
                                               "Packages are generated at 4am UTC daily.\n\n"
                                               "Do you want to re-install the latest database update anyway?"));

        if(button == QMessageBox::Yes)
        {
            doInstallation(remote);
        }
    }).finally([this]{
        this->setEnabled(true);
    });
}

void AeqSelector::deleteDatabase()
{
    pkgManager->uninstallPackage();
    QMessageBox::information(this, tr("Database cleared"), tr("The database has been removed from your hard disk"));
    reject();
}

void AeqSelector::updateDatabaseInfo()
{
    ui->searchInput->setText("");
    ui->list->selectionModel()->clearSelection();

    pkgManager->getLocalVersion().then([this](AeqVersion local){
        ui->db_commit->setText(local.commit.left(7));
        ui->db_committime->setText(local.commitTime.toString("yy/MM/dd HH:mm:ss") + " UTC");
        ui->db_uploadtime->setText(local.packageTime.toString("yy/MM/dd HH:mm:ss") + " UTC");
    }).fail([this]{
        ui->db_commit->setText("N.A.");
        ui->db_committime->setText("N.A.");
        ui->db_uploadtime->setText("N.A.");
    });

    pkgManager->getLocalIndex().then([this](const QVector<AeqMeasurement>& items){
        model->import(items);
    });
}

void AeqSelector::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)
    ui->previewStack->setCurrentIndex(!selected.isEmpty());
    if(ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok) != nullptr)
        ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(!selected.isEmpty());

    if(selected.isEmpty())
    {
        return;
    }

    auto csv = this->selection(DataFormat::dCsv, true);
    auto item = proxyModel->data(ui->list->selectionModel()->selectedRows().first(), Qt::UserRole).value<AeqMeasurement>();
    auto title = QString("%1 (%2)").arg(item.name, item.source);
    if(!csv.isEmpty())
    {
        ui->preview->importCsv(csv, title);
    }
    else
    {
        auto graphic = this->selection(DataFormat::dGraphicEq, false);
        ui->preview->importGraphicEq(graphic, title);
    }
}

void AeqSelector::onDarkModeToggled(int state)
{
    AppConfig::instance().set(AppConfig::AeqPlotDarkMode, (bool)state);
    // Reload plot
    onSelectionChanged(ui->list->selectionModel()->selection(), QItemSelection());
}

QString AeqSelector::selection(DataFormat format, bool silent)
{
    if (ui->list->selectionModel()->selectedRows().isEmpty())
	{
        return QString();
	}

    auto item = proxyModel->data(ui->list->selectionModel()->selectedRows().first(), Qt::UserRole).value<AeqMeasurement>();

    QString name;
    switch(format)
    {
    case AeqSelector::dGraphicEq:
        name = "graphic.txt";
        break;
    case AeqSelector::dCsv:
        name = "raw.csv";
        break;
    }

    QFile file(item.path(pkgManager->databaseDirectory(), name));
    if(!file.exists())
    {
        if(!silent)
        {
            QMessageBox::critical(this, tr("Error"), tr("Unable to retrieve corresponding file from database. Please update the local database as it appears to be incomplete."));
        }
        return QString();
    }

    auto guard = qScopeGuard([&]{ file.close(); });
    file.open(QFile::ReadOnly);
    return file.readAll();
}
