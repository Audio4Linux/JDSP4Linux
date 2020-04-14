#ifndef EELCOMPLETER_H
#define EELCOMPLETER_H
// Qt
#include <QCompleter> // Required for inheritance
#include <QStandardItemModel>

#include "model/functiondefinition.h"

/**
 * @brief Class, that describes completer with
 * eel specific types and functions.
 */
class EELCompleter : public QCompleter
{
    Q_OBJECT

public:

    /**
     * @brief Constructor.
     * @param parent Pointer to parent QObject.
     */
    explicit EELCompleter(QObject* parent=nullptr);

    void insertBuiltinItem(QString key, QString name, int row);
    void removeCustomItems();
    void appendDefinitions(QList<FunctionDefinition> defs);
    void removeStubbedDefinitions(QList<FunctionDefinition> currentdefs);
    void appendDefinitionsIfMissing(QList<FunctionDefinition> defs);
private:
    QStandardItemModel* smodel;
};


#endif // EELCOMPLETER_H
