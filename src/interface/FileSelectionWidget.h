#ifndef FILESELECTIONWIDGET_H
#define FILESELECTIONWIDGET_H

#include <QWidget>
#include <QDir>
#include <optional>

namespace Ui {
class FileSelectionWidget;
}

class FileSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileSelectionWidget(QWidget *parent = nullptr);
    ~FileSelectionWidget();

    void setCurrentDirectory(const QDir &path);
    QDir currentDirectory() const;

    void setCurrentFile(const QString &path);
    std::optional<QString> currentFile() const;
    std::optional<QString> currentFileName() const;

    std::optional<QDir> bookmarkDirectory() const;
    void setBookmarkDirectory(const std::optional<QDir> &newBookmarkDirectory);

    QStringList fileTypes() const;
    void setFileTypes(const QStringList &newFileTypes);

    bool fileActionsVisible() const;
    void setFileActionsVisible(bool visible);

    bool navigationBarVisible() const;
    void setNavigationBarVisible(bool visible);

public slots:
    void clearCurrentFile();
    void enumerateFiles();

signals:
    void fileChanged(const QString& path);
    void bookmarkAdded(const QString& path);

private slots:
    void onFileSelectionChanged();
    void onRenameRequested();
    void onRemoveRequested();
    void onDirectoryChangeRequested();
    void onBookmarkRequested();

private:
    Ui::FileSelectionWidget *ui;

    std::optional<QDir>  _bookmarkDirectory;
    QStringList _fileTypes;
    std::optional<QString> _currentFile;

    void setContextButtonsAvailable(bool b);
    void setBottomActionsVisible(bool b);
};

#endif // FILESELECTIONWIDGET_H
