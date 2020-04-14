#ifndef EELEDITOR_H
#define EELEDITOR_H

#include <QDialog>
#include "model/functiondefinition.h"
#include "model/annotationdefinition.h"
#include "model/customsymbolprovider.h"

#include "model/eelcompleter.h"
#include "model/eelhighlighter.h"

QT_BEGIN_NAMESPACE
namespace Ui { class EELEditor; }
QT_END_NAMESPACE

class EELEditor : public QDialog
{
    Q_OBJECT

public:
    EELEditor(QWidget *parent = nullptr);
    ~EELEditor();
    void openNewScript(QString path);

signals:
    void scriptSaved(QString path);

private:
    Ui::EELEditor *ui;
    CustomSymbolProvider* symbolProvider;
    EELHighlighter* highlighter;
    EELCompleter* completer;
};
#endif // EELEDITOR_H
