#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include "ui_settings.h"
class settings : public QDialog
{
    Q_OBJECT

public:
    settings(QWidget *parent = nullptr);
    Ui::settings *ui;
    ~settings();
private slots:
        void openPalConfig();
        void reject();
        void github();
        void glava_help();
        void changeStyle(const QString&);
        void changeThemeMode();
        void changePalette(const QString&);
        void updateAutoFX();
        void updateReloadMethod();
        void updatePath();
        void updateGLava();
        void updateAutoFxMode();
        void updateTheme();
};

#endif // SETTINGS_H
