#ifndef FILEDOWNLOADERDIALOG_H
#define FILEDOWNLOADERDIALOG_H

#include <QDialog>

namespace Ui {
class FileDownloaderDialog;
}

class FileDownloaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FileDownloaderDialog(QWidget *parent = nullptr);
    ~FileDownloaderDialog();

private:
    Ui::FileDownloaderDialog *ui;
};

#endif // FILEDOWNLOADERDIALOG_H
