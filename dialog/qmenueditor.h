#ifndef MENUEDITOR_H
#define MENUEDITOR_H

#include <QWidget>
#include <QMenu>
#include <QListWidget>

namespace Ui {
class menueditor;
}

class QMenuEditor : public QWidget
{
    Q_OBJECT

public:
    explicit QMenuEditor(QWidget *parent = nullptr);
    ~QMenuEditor();

   void setSourceMenu(QMenu *source);
   void setTargetMenu(QMenu *target);
   QMenu *exportMenu();
   void setIconStyle(bool white);
signals:
   void targetChanged();
   void resetPressed();
private:
    Ui::menueditor *ui;
    QList<QAction*> mTarget;
    QList<QAction*> mSource;

    bool insertAction(QAction *new_action);
    void populateList(QList<QAction*> menu, QListWidget *list);
};

#endif // MENUEDITOR_H
