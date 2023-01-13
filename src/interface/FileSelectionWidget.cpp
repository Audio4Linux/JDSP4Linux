#include "FileSelectionWidget.h"
#include "ui_FileSelectionWidget.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>

FileSelectionWidget::FileSelectionWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FileSelectionWidget)
{
    ui->setupUi(this);

    setContextButtonsAvailable(false);
    setFileActionsVisible(false);
    setBookmarkDirectory(std::nullopt);

    connect(ui->fileview, &QListWidget::itemSelectionChanged, this, &FileSelectionWidget::onFileSelectionChanged);

    connect(ui->rename, &QPushButton::clicked, this, &FileSelectionWidget::onRenameRequested);
    connect(ui->remove, &QPushButton::clicked, this, &FileSelectionWidget::onRemoveRequested);
    connect(ui->bookmark, &QPushButton::clicked, this, &FileSelectionWidget::onBookmarkRequested);
    connect(ui->changedir, &QPushButton::clicked, this, &FileSelectionWidget::onDirectoryChangeRequested);
    connect(ui->reload, &QPushButton::clicked, this, &FileSelectionWidget::enumerateFiles);
}

FileSelectionWidget::~FileSelectionWidget()
{
    delete ui;
}

void FileSelectionWidget::setCurrentDirectory(const QDir &path)
{
    ui->path->setText(path.path());
    enumerateFiles();
}

QDir FileSelectionWidget::currentDirectory() const
{
    return QDir(ui->path->text());
}

void FileSelectionWidget::clearCurrentFile()
{
    _currentFile = std::nullopt;
    ui->fileview->clearSelection();
    setContextButtonsAvailable(false);
}

void FileSelectionWidget::setCurrentFile(const QString& path)
{
    if(path.isEmpty())
    {
        _currentFile = std::nullopt;
    }
    else
    {
        setCurrentDirectory(QFileInfo(path).absoluteDir().absolutePath());
        _currentFile = path;
    }

    enumerateFiles();
}

std::optional<QString> FileSelectionWidget::currentFile() const
{
    return _currentFile;
}

std::optional<QString> FileSelectionWidget::currentFileName() const
{
    if(!_currentFile.has_value())
    {
        return std::nullopt;
    }

    return QFileInfo(_currentFile.value()).fileName();
}

void FileSelectionWidget::enumerateFiles()
{
    ui->fileview->blockSignals(true);
    ui->fileview->clear();

    setContextButtonsAvailable(false);

    QStringList files = currentDirectory().entryList(_fileTypes);
    if (files.empty())
    {
        QListWidgetItem *placeholder = new QListWidgetItem;
        placeholder->setText(tr("No supported files found"));
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
        ui->fileview->addItem(placeholder);
    }
    else
    {
        ui->fileview->addItems(files);
        ui->fileview->clearSelection();

        if (_currentFile.has_value() && QFile(_currentFile.value()).exists())
        {
            for (int i = 0; i < ui->fileview->count(); i++)
            {
                if (ui->fileview->item(i)->text() == currentFileName().value())
                {
                    ui->fileview->setCurrentRow(i);
                    setContextButtonsAvailable(true);
                    break;
                }
            }
        }
    }

    ui->fileview->blockSignals(false);
}

bool FileSelectionWidget::navigationBarVisible() const
{
    return ui->navigationBarContainer->isVisible();
}

void FileSelectionWidget::setNavigationBarVisible(bool visible)
{
    ui->navigationBarContainer->setVisible(visible);
}

bool FileSelectionWidget::fileActionsVisible() const
{
    return ui->fileActionContainer->isVisible();
}

void FileSelectionWidget::setFileActionsVisible(bool visible)
{
    ui->fileActionContainer->setVisible(visible);
}

void FileSelectionWidget::onDirectoryChangeRequested()
{
    auto* dialog = new QFileDialog();
    dialog->setDirectory(currentDirectory());
    dialog->setFileMode(QFileDialog::Directory);
    dialog->setOption(QFileDialog::ShowDirsOnly);
    dialog->setViewMode(QFileDialog::Detail);

    QString result = dialog->getExistingDirectory();
    if (result != "")
    {
        ui->path->setText(result);
        enumerateFiles();
    }
}

void FileSelectionWidget::onBookmarkRequested()
{
    if(ui->fileview->selectedItems().count() < 1 || !bookmarkDirectory().has_value())
    {
        return;
    }

    QString src  = currentDirectory().filePath(currentFile().value());
    QString dest = bookmarkDirectory().value().filePath(currentFileName().value());

    if (QFile::exists(dest))
    {
        QFile::remove(dest);
    }

    QFile::copy(src, dest);
    enumerateFiles();

    emit bookmarkAdded(dest);
}

void FileSelectionWidget::onFileSelectionChanged()
{
    if(ui->fileview->selectedItems().count() < 1)
    {
        setContextButtonsAvailable(false);
        return;
    }

    setContextButtonsAvailable(true);

    QString path = currentDirectory().filePath(ui->fileview->selectedItems().first()->text());

    if (QFileInfo::exists(path) && QFileInfo(path).isFile())
    {
        _currentFile = path;
        emit fileChanged(path);
    }
}

void FileSelectionWidget::onRenameRequested()
{
    if (ui->fileview->selectedItems().count() < 1)
    {
        return;
    }

    bool ok;
    QString name = QInputDialog::getText(this, "Rename",
                                         "New name", QLineEdit::Normal,
                                         ui->fileview->selectedItems().first()->text(), &ok);
    QString src  = currentDirectory().filePath(currentFile().value());
    QString dest = currentDirectory().filePath(name);

    if (ok && !name.isEmpty())
    {
        QFile::rename(src, dest);
    }

    enumerateFiles();
}

void FileSelectionWidget::onRemoveRequested()
{
    if (ui->fileview->selectedItems().count() < 1)
    {
        return;
    }

    QString path = currentDirectory().filePath(currentFile().value());

    if (QFile::exists(path))
    {
        QFile(path).remove();
    }

    enumerateFiles();
}

std::optional<QDir> FileSelectionWidget::bookmarkDirectory() const
{
    return _bookmarkDirectory;
}

void FileSelectionWidget::setBookmarkDirectory(const std::optional<QDir> &newBookmarkDirectory)
{
    _bookmarkDirectory = newBookmarkDirectory;

    ui->bookmark->setVisible(_bookmarkDirectory.has_value());
}

QStringList FileSelectionWidget::fileTypes() const
{
    return _fileTypes;
}

void FileSelectionWidget::setFileTypes(const QStringList &newFileTypes)
{
    _fileTypes = newFileTypes;
    enumerateFiles();
}

void FileSelectionWidget::setContextButtonsAvailable(bool b)
{
    ui->rename->setEnabled(b);
    ui->remove->setEnabled(b);
    ui->bookmark->setEnabled(b);
}

