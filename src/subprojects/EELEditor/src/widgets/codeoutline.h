#ifndef CODEOUTLINE_H
#define CODEOUTLINE_H

#include <QTreeWidget>
#include <QMutex>
#include "model/functiondefinition.h"
#include "model/annotationdefinition.h"

class CodeOutline : public QTreeWidget
{
    Q_OBJECT
public:
    CodeOutline(QWidget *parent = nullptr);
    void updateFunctions(QList<FunctionDefinition> defs);
    void updateAnnotations(QList<AnnotationDefinition> defs);
    template<typename T>
    void updateField(QTreeWidgetItem *rootView, QList<T> defs);
    bool goToAnnotation(QString name);
    bool goToFunction(QString name);
    bool goToFunctionByLine(int line, bool noSync = true);
signals:
    void annotationSelected(AnnotationDefinition);
    void functionSelected(FunctionDefinition);
private:
    QTreeWidgetItem* twAnnotation;
    QTreeWidgetItem* twFunctions;
    bool disableChangeSignal = false;
};

#endif // CODEOUTLINE_H
