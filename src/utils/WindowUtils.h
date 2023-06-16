#ifndef WINDOWUTILS_H
#define WINDOWUTILS_H

#include <QMainWindow>
#include <QApplication>
#include "Log.h"

namespace WindowUtils {

QMainWindow* getMainWindow()
{
    foreach (QWidget *w, qApp->topLevelWidgets())
        if (QMainWindow* mainWin = qobject_cast<QMainWindow*>(w))
            return mainWin;

    Log::error("QMainWindow not found");
    return nullptr;
}

};

#endif // WINDOWUTILS_H
