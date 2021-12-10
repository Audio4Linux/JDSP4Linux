#ifndef AUTOEQSELECTOR_H
#define AUTOEQSELECTOR_H

#include <QDialog>
#include <QThread>

namespace Ui
{
	class AutoEQSelector;
}

class AutoEQSelector :
	public QDialog
{
	Q_OBJECT

public:
	explicit AutoEQSelector(QWidget *parent = nullptr);
	~AutoEQSelector();
    //HeadphoneMeasurement getSelection();

protected:
    //void                 appendToList(QueryResult result);

private slots:
	void                 updateDetails();
	void                 doQuery();

private:
	Ui::AutoEQSelector *ui;
	QSize imgSizeCache;
    //HeadphoneMeasurement hpCache;
};

#endif // AUTOEQSELECTOR_H
