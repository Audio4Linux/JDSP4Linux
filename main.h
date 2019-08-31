#include "mainwindow.h"
#include "preset.h"
#ifndef MAIN_H
#define MAIN_H
#ifdef MAIN
  #define EXTERN
#else
  #define EXTERN extern
#endif
EXTERN MainWindow *mainwin;
EXTERN QApplication *app;
EXTERN Preset *preset;
#endif // MAIN_H
