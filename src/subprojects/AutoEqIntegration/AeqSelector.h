#ifndef AEQSELECTOR_H
#define AEQSELECTOR_H

#include <QDialog>
#include <QItemSelection>

class AeqMeasurementModel;
class AeqPackageManager;
class QSortFilterProxyModel;

namespace Ui
{
    class AeqSelector;
}

class AeqSelector :
	public QDialog
{
	Q_OBJECT

public:
    enum DataFormat {
        dGraphicEq,
        dCsv
    };

    explicit AeqSelector(QWidget *parent = nullptr);
    ~AeqSelector();

    QString selection(DataFormat format);

protected:
    void showEvent(QShowEvent *) override;

private slots:
    void switchPane();
    void updateDatabase();
    void deleteDatabase();
    void updateDatabaseInfo();
    void onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private:
    Ui::AeqSelector *ui;
    AeqPackageManager *pkgManager;
    AeqMeasurementModel *model;
    QSortFilterProxyModel *proxyModel;
};

#endif // AEQSELECTOR_H
