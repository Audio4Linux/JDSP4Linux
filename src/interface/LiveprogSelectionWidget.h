#ifndef LIVEPROGSELECTIONWIDGET_H
#define LIVEPROGSELECTIONWIDGET_H

#include <QWidget>

class EELParser;
class EELEditor;

namespace Ui {
class LiveprogSelectionWidget;
}

class LiveprogSelectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LiveprogSelectionWidget(QWidget *parent = nullptr);
    ~LiveprogSelectionWidget();

    void coupleIDE(EELEditor *editor);
    void updateList();

    bool isActive() const;
    void setActive(bool active);

    const QString &currentLiveprog() const;
    void setCurrentLiveprog(const QString &path);

signals:
    void liveprogReloadRequested();
    void toggled(bool on);
    void scriptChanged(const QString& path);
    void unitLabelUpdateRequested(const QString& msg);

public slots:
    void updateFromEelEditor(QString path);

private slots:
    void onEditScript();
    void onResetLiveprogParams();
    void onFileChanged(const QString &path);

private:
    Ui::LiveprogSelectionWidget *ui;

    EELParser *_eelParser;
    EELEditor *_eelEditor = nullptr;
    QString _currentLiveprog;

    void loadProperties(const QString& path);
};

#endif // LIVEPROGSELECTIONWIDGET_H
