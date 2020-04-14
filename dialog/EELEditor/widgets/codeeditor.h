#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QTimer>
#include <QCodeEditor.hpp>
#include <model/annotationdefinition.h>
#include <model/codecontainer.h>
#include <model/functiondefinition.h>

class CodeEditor : public QCodeEditor
{
    Q_OBJECT
public:
    CodeEditor(QWidget* parent = nullptr);
    void goToLine(int line);
    QList<FunctionDefinition> findFunctions();
    QList<AnnotationDefinition> findAnnotations();
    void loadStyle(QString path);
    void loadCodeFromFile(QString path);
    void loadCode(CodeContainer *code);
    int getCurrentLine();
    QString textUnderCursor() const;
signals:
    void backendRefreshRequired();
    void cursorRefreshed();
private slots:
    void fireRefreshSignal();
private:
    QTimer* refreshTick;
    bool refreshSignalQueued = false;
    bool refreshCursorSignalQueued = false;
};

#endif // CODEEDITOR_H
