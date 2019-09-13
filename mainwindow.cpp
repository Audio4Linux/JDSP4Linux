#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settings.h"
#include "preset.h"
#include "ui_settings.h"
#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <map>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include "configlist.h"
#include <ostream>
#include <QApplication>
#include <QMessageBox>
#include <utility>
#include <QProcess>
#include <QDebug>
#include <QClipboard>
#include "main.h"
#include <vector>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QFileDialog>
#include <cctype>
#include <cmath>
#include <QStyleFactory>
#include <QWhatsThis>
#include "log.h"
#include <QObject>
#include <QtSql>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
using namespace std;
static string path = "";
static string appcpath = "";
static string style_sheet = "";
static string color_palette = "";
static string custom_palette = "";
static string theme_str = "";
static int theme_mode = 0;
static int autofxmode = 0;
static bool bd_padding = 0;
static bool custom_whiteicons = false;
static bool autofx = false;
static bool glava_fix = false;
static bool settingsdlg_enabled=true;
static bool presetdlg_enabled=true;
static bool logdlg_enabled=true;
static bool lockapply = false;
static bool lockddcupdate = false;
static QString ddcpath = "";
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Clear log
    QFile file ("/tmp/jamesdsp/ui.log");
    if(file.exists())file.remove();
    QFile file2 ("/tmp/jamesdsp/ui_jdsp.log");
    if(file2.exists())file2.remove();

    writeLog("UI launched...");

    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    char result[100];
    strcpy(result,homedir);
    strcat(result,"/.config/jamesdsp/audio.conf");
    char result2[100];
    strcpy(result2,homedir);
    strcat(result2,"/.config/jamesdsp/ui.conf");
    path = result;
    appcpath = result2;

    reloadConfig();
    loadAppConfig();

    QDir d = QFileInfo(QString::fromStdString(getPath())).absoluteDir();
    QString absolute=d.absolutePath();

    if(ddcpath==""||ddcpath==(absolute+"/dbcopy.vdc")){
        ui->ddc_dirpath->setText(QString::fromStdString(homedir));
        reloadDDC();
        if(ddcpath==(absolute+"/dbcopy.vdc"))ui->ddcTabs->setCurrentIndex(1);
    }else{
        try {
            QDir d2 = QFileInfo(ddcpath).absoluteDir();
            ui->ddc_dirpath->setText(d2.absolutePath());
            reloadDDC();
            if(ui->ddc_files->count() >= 1){
                for(int i=0;i<ui->ddc_files->count();i++){
                    if(ui->ddc_files->item(i)->text()==QFileInfo(ddcpath).fileName()){
                        ui->ddc_files->setCurrentRow(i);
                        break;
                    }
                }
            }
        } catch (exception e) {
            writeLog("Failed to load previous DDC path: " + QString::fromStdString(e.what()));
            ui->ddc_dirpath->setText(QString::fromStdString(homedir));
            reloadDDC();
        }
    }


    QMenu *menu = new QMenu();
    menu->addAction("Reload JDSP", this,SLOT(Restart()));
    menu->addAction("Context Help", this,[this](){QWhatsThis::enterWhatsThisMode();});
    menu->addAction("View Logs", this,SLOT(OpenLog()));
    menu->addAction("Load from file", this,SLOT(LoadExternalFile()));
    menu->addAction("Save to file", this,SLOT(SaveExternalFile()));

    ui->toolButton->setMenu(menu);

    if (createconnection()){
        model = new QSqlTableModel;

        model = new QSqlTableModel(this);
        model->setTable("DDCData");
        model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        model->select();

        model->setHeaderData(0, Qt::Horizontal, tr("ID"));
        model->setHeaderData(1, Qt::Horizontal, tr("Vendor"));
        model->setHeaderData(2, Qt::Horizontal, tr("CTranslate"));
        model->setHeaderData(3, Qt::Horizontal, tr("Model"));
        model->setHeaderData(4, Qt::Horizontal, tr("MTranslate"));
        model->setHeaderData(5, Qt::Horizontal, tr("SR_44100_Coeffs"));
        model->setHeaderData(6, Qt::Horizontal, tr("SR_48000_Coeffs"));

        ui->ddcTable->setModel(model);
        ui->ddcTable->setColumnHidden(0, true);
        ui->ddcTable->setColumnHidden(2, true);
        ui->ddcTable->setColumnHidden(4, true);
        ui->ddcTable->setColumnHidden(5, true);
        ui->ddcTable->setColumnHidden(6, true);
        ui->ddcTable->resizeColumnsToContents();
    }


    SetStyle();
    ConnectActions();
}

MainWindow::~MainWindow()
{
    delete ui;
}
//SQL Loader
void MainWindow::updateDDC(const QItemSelection &item, const QItemSelection &item2){
    QItemSelectionModel *select = ui->ddcTable->selectionModel();
    QString ddc_coeffs;
    if(select->hasSelection()){
        lockddcupdate=true;
        ui->ddc_files->clearSelection();
        lockddcupdate=false;
        int index = select->selectedRows().first().row();
        ddc_coeffs += "SR_44100:";
        ddc_coeffs += ui->ddcTable->model()->data(ui->ddcTable->model()->index(index,5)).toString();
        ddc_coeffs += "\nSR_48000:";
        ddc_coeffs += ui->ddcTable->model()->data(ui->ddcTable->model()->index(index,6)).toString();
        QDir d = QFileInfo(QString::fromStdString(getPath())).absoluteDir();
        QString absolute=d.absolutePath();
        QFile file(absolute+"/dbcopy.vdc");
        if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(ddc_coeffs.toUtf8().constData());
        }
        file.close();

        ddcpath = absolute + "/dbcopy.vdc";
    }
    OnUpdate();
}

bool MainWindow::createconnection()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QDir d = QFileInfo(QString::fromStdString(getPath())).absoluteDir();
    QString absolute=d.absolutePath();

    if(QFile::exists(absolute+"/ViPERDDC.db"))QFile::remove(absolute+"/ViPERDDC.db");
    QFile::copy(":/ddc/ViPERDDC.db", absolute+"/ViPERDDC.db");
    db.setDatabaseName(absolute+"/ViPERDDC.db");
    if (!db.open()) {
        writeLog("Cannot open DDC database. Unable to establish a database connection.");
        return false;
    }

    QSqlQuery query;
    query.exec("SELECT * FROM 'DDCData'");

    return true;
}

//--Logs
void MainWindow::writeLog(const QString& log,int mode){
    //Mode: 0-Log+Stdout 1-Log 2-Stdout
    QFile f("/tmp/jamesdsp/ui.log");
    QString o = "[" + QTime::currentTime().toString() + "] " + log + "\n";
    QString o_alt = "[" + QTime::currentTime().toString() + "] " + log;

    if(mode==0||mode==1){
        if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
            f.write(o.toUtf8().constData());
        }
        f.close();
    }
    if(mode==0||mode==2) cout << o_alt.toUtf8().constData() << endl;
}
void MainWindow::writeLogF(const QString& log,const QString& _path){
    QFile f(_path);
    QString o = "[" + QTime::currentTime().toString() + "] " + log;
    if (f.open(QIODevice::WriteOnly | QIODevice::Append)) {
        f.write(o.toUtf8().constData());
    }
    f.close();
}

//---Style
void MainWindow::SetStyle(){
    QApplication::setStyle(QString::fromStdString(mainwin->getTheme()));

    if(theme_mode==0){
        QApplication::setPalette(this->style()->standardPalette());
        QString stylepath = "";
        if (style_sheet=="blue")stylepath = ":darkblue/darkblue/darkblue.qss";
        else if (style_sheet=="breeze_light")stylepath = ":/lightbreeze/lightbreeze/lightbreeze.qss";
        else if (style_sheet=="breeze_dark")stylepath = ":/darkbreeze/darkbreeze/darkbreeze.qss";
        else if (style_sheet=="amoled")stylepath = ":amoled/amoled/amoled.qss";
        else if (style_sheet=="aqua")stylepath = ":/aqua/aqua/aqua.qss";
        else if (style_sheet=="materialdark")stylepath = ":/materialdark/materialdark/materialdark.qss";
        else if (style_sheet=="ubuntu")stylepath = ":/ubuntu/ubuntu/ubuntu.qss";
        else if (style_sheet=="vsdark")stylepath = ":/vsdark/vsdark/vsdark.qss";
        else if (style_sheet=="vslight")stylepath = ":/vslight/vslight/vslight.qss";
        else stylepath = ":/default.qss";
        QFile f(stylepath);
        if (!f.exists())printf("Unable to set stylesheet, file not found\n");
        else
        {
            f.open(QFile::ReadOnly | QFile::Text);
            QTextStream ts(&f);
            qApp->setStyleSheet(ts.readAll());
            if(style_sheet=="amoled" || style_sheet=="console" || style_sheet=="materialdark" || style_sheet=="breeze_dark" || style_sheet=="vsdark"){
                QPixmap pix(":/icons/settings-white.svg");
                QIcon icon(pix);
                QPixmap pix2(":/icons/queue-white.svg");
                QIcon icon2(pix2);
                QPixmap pix3(":/icons/menu-white.svg");
                QIcon icon3(pix3);
                ui->set->setIcon(icon);
                ui->cpreset->setIcon(icon2);
                ui->toolButton->setIcon(icon3);
            }else if (getWhiteIcons()) {
                loadIcons(getWhiteIcons());
            }
            else{
                QPixmap pix(":/icons/settings.svg");
                QIcon icon(pix);
                QPixmap pix2(":/icons/queue.svg");
                QIcon icon2(pix2);
                QPixmap pix3(":/icons/menu.svg");
                QIcon icon3(pix3);
                ui->set->setIcon(icon);
                ui->cpreset->setIcon(icon2);
                ui->toolButton->setIcon(icon3);
            }
        }

    }else{
        loadIcons(false);
        if(color_palette=="dark"){
            QColor background = QColor(53,53,53);
            QColor foreground = Qt::white;
            QColor base = QColor(25,25,25);
            QColor selection = QColor(42, 130, 218);
            setPalette(base,background,foreground,selection,Qt::black);
        }else if(color_palette=="purple"){
            loadIcons(true);
            QColor background = QColor(26, 0, 25);
            QColor foreground = Qt::white;
            QColor base = QColor(23, 0, 19);
            QColor selection = QColor(42, 130, 218);
            setPalette(base,background,foreground,selection,Qt::black);
        }else if(color_palette=="gray"){
            loadIcons(true);
            QColor background = QColor(49,49,74);
            QColor foreground = Qt::white;
            QColor base = QColor(83,83,125);
            QColor selection = QColor(85,85,127);
            setPalette(base,background,foreground,selection,Qt::black,QColor(144,144,179));
        }else if(color_palette=="white"){
            QColor background = Qt::white;
            QColor foreground = Qt::black;
            QColor base = Qt::white;
            QColor selection = QColor(56,161,227);
            setPalette(base,background,foreground,selection,Qt::black);
        }
        else if(color_palette=="blue"){
            loadIcons(true);
            QColor background = QColor(0,0,50);
            QColor foreground = Qt::white;
            QColor base = QColor(0,0,38);
            QColor selection = QColor(85,0,255);
            setPalette(base,background,foreground,selection,Qt::black);
        }
        else if(color_palette=="darkblue"){
            loadIcons(true);
            QColor background = QColor(19,25,38);
            QColor foreground = Qt::white;
            QColor base = QColor(14,19,29);
            QColor selection = QColor(70,79,89);
            setPalette(base,background,foreground,selection,Qt::black);
        }
        else if(color_palette=="honeycomb"){
            QColor background = QColor(212,215,208);
            QColor foreground = Qt::black;
            QColor base = QColor(185,188,182);
            QColor selection = QColor(243,193,41);
            setPalette(base,background,foreground,selection,Qt::white);
        }
        else if(color_palette=="black"){
            loadIcons(true);
            QColor background = QColor(16,16,16);
            QColor foreground = QColor(222,222,222);
            QColor base = Qt::black;
            QColor selection = QColor(132,132,132);
            setPalette(base,background,foreground,selection,Qt::black);
        }
        else if(color_palette=="solarized"){
            loadIcons(true);
            QColor background = QColor(15,30,49);
            QColor foreground = QColor(154,174,180);
            QColor base = Qt::black;
            QColor selection = QColor(3,50,63);
            setPalette(base,background,foreground,selection,Qt::black);
        }
        else if(color_palette=="silver"){
            QColor background = QColor(176,180,196);
            QColor foreground = QColor(20,20,20);
            QColor base = QColor(176,180,196);
            QColor selection = Qt::white;
            setPalette(base,background,foreground,selection,Qt::black);
        }
        else if(color_palette=="darkgreen"){
            loadIcons(true);
            QColor background = QColor(27,34,36);
            QColor foreground = QColor(197,209,217);
            QColor base = QColor(30,30,30);
            QColor selection = QColor(21,67,58);
            setPalette(base,background,foreground,selection,Qt::black);
        }
        else if(color_palette=="custom"){
            QColor base = QColor(loadColor(0,0),loadColor(0,1),loadColor(0,2));
            QColor background = QColor(loadColor(1,0),loadColor(1,1),loadColor(1,2));
            QColor foreground = QColor(loadColor(2,0),loadColor(2,1),loadColor(2,2));
            QColor selection = QColor(loadColor(3,0),loadColor(3,1),loadColor(3,2));
            QColor disabled = QColor(loadColor(4,0),loadColor(4,1),loadColor(4,2));
            QColor selectiontext = QColor(255-loadColor(3,0),255-loadColor(3,1),255-loadColor(3,2));

            setPalette(base,background,foreground,selection,selectiontext,disabled);
            loadIcons(getWhiteIcons());
        }
        else{
            QApplication::setPalette(this->style()->standardPalette());
            QFile f(":/default.qss");
            if (!f.exists())printf("Unable to set stylesheet, file not found\n");
            else
            {
                f.open(QFile::ReadOnly | QFile::Text);
                QTextStream ts(&f);
                qApp->setStyleSheet(ts.readAll());
            }
        }

    }
}
void MainWindow::setPalette(const QColor& base,const QColor& background,const QColor& foreground,const QColor& selection = QColor(42,130,218),const QColor& selectiontext = Qt::black,const QColor& disabled){
    QPalette *palette = new QPalette();
    palette->setColor(QPalette::Window, background);
    palette->setColor(QPalette::WindowText, foreground);
    palette->setColor(QPalette::Base, base);
    palette->setColor(QPalette::AlternateBase, background);
    palette->setColor(QPalette::ToolTipBase, background);
    palette->setColor(QPalette::ToolTipText, foreground);
    palette->setColor(QPalette::Text, foreground);
    palette->setColor(QPalette::Button, background);
    palette->setColor(QPalette::ButtonText, foreground);
    palette->setColor(QPalette::BrightText, Qt::red);
    palette->setColor(QPalette::Link, QColor(42, 130, 218));
    palette->setColor(QPalette::Highlight, selection);
    palette->setColor(QPalette::HighlightedText, selectiontext);
    app->setPalette(*palette);
    QString rgbdisabled = disabled.name();
    app->setStyleSheet("QFrame[frameShape=\"4\"], QFrame[frameShape=\"5\"]{ color: gray; }*::disabled { color: " + rgbdisabled +";}QToolButton::disabled { color: " + rgbdisabled +";}QComboBox::disabled { color: " + rgbdisabled +";}");
}
void MainWindow::loadIcons(bool white){
    if(white){
        QPixmap pix(":/icons/settings-white.svg");
        QIcon icon(pix);
        QPixmap pix2(":/icons/queue-white.svg");
        QIcon icon2(pix2);
        QPixmap pix3(":/icons/menu-white.svg");
        QIcon icon3(pix3);
        ui->set->setIcon(icon);
        ui->cpreset->setIcon(icon2);
        ui->toolButton->setIcon(icon3);
    }else{
        QPixmap pix(":/icons/settings.svg");
        QIcon icon(pix);
        QPixmap pix2(":/icons/queue.svg");
        QIcon icon2(pix2);
        QPixmap pix3(":/icons/menu.svg");
        QIcon icon3(pix3);
        ui->set->setIcon(icon);
        ui->cpreset->setIcon(icon2);
        ui->toolButton->setIcon(icon3);
    }
}
int MainWindow::loadColor(int index,int rgb_index){
    QStringList elements = QString::fromStdString(mainwin->getCustompalette()).split(';');
    if(elements.length()<5||elements[index].split(',').size()<3){
        if(index==0)return 25;
        else if(index==1)return 53;
        else if(index==2)return 255;
        else if(index==3){
            if(rgb_index==0)return 42;
            else if(rgb_index==1)return 130;
            else if(rgb_index==2)return 218;
        }
        else if(index==4) return 85;
    }
    QStringList rgb = elements[index].split(',');
    return rgb[rgb_index].toInt();
}
void MainWindow::switchPalette(const QPalette& palette)
{
    app->setPalette(palette);

    QList<QWidget*> widgets = this->findChildren<QWidget*>();

    foreach (QWidget* w, widgets)
    {
        w->setPalette(palette);
    }
}

//---Dialogs
void MainWindow::OpenLog(){
    if(logdlg_enabled){
        enableLogBtn(false);
        auto c = new class log(this);
        //c->setFixedSize(c->geometry().width(),c->geometry().height());
        c->show();
    }
}
void MainWindow::OpenSettings(){
    if(settingsdlg_enabled){
        enableSetBtn(false);
        auto setting = new settings(this);
        //setting->setFixedSize(setting->geometry().width(),setting->geometry().height());
        setting->show();
    }
}
void MainWindow::OpenPreset(){
    if(presetdlg_enabled){
        enablePresetBtn(false);
        auto _preset = new Preset(this);
        preset = _preset;
        //_preset->setFixedSize(_preset->geometry().width(),_preset->geometry().height());
        _preset->show();
    }
}
void MainWindow::enableSetBtn(bool on){
    settingsdlg_enabled=on;
}
void MainWindow::enablePresetBtn(bool on){
    presetdlg_enabled=on;
}
void MainWindow::enableLogBtn(bool on){
    logdlg_enabled=on;
}

//---Apply
void MainWindow::OnUpdate(bool ignoremode){
    //Will be called when slider has been moved, dynsys/eq preset set or spinbox changed
    if((autofx&&(ignoremode||autofxmode==0)) && !lockapply){
        ConfirmConf();
    }
}
void MainWindow::OnRelease(){
    if((autofx&&(autofxmode==1)) && !lockapply){
        ConfirmConf();
    }
}
void MainWindow::ConfirmConf(bool restart){
    string config = "enable=";
    if(!ui->disableFX->isChecked())config += "true\n";
    else config += "false\n";

    config += getBass();
    config += getSurround();
    config += getEQ();
    config += getComp();
    config += getMisc();

    ofstream myfile(path);
    if (myfile.is_open())
    {
        myfile << config;
        //writeLog("Updating JDSP Config... (main/writer)");
        myfile.close();
    }
    else writeLog("Unable to write to '" + QString::fromStdString(path) + "'; cannot update jdsp config (main/confirmconf)");
    if(restart)Restart();
}
void MainWindow::Reset(){
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"Reset Configuration","Are you sure?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        std::filebuf fb;
        fb.open (path,std::ios::out);
        std::ostream os(&fb);
        os << default_config;
        fb.close();

        reloadConfig();
        ConfirmConf();
    }
}
void MainWindow::Restart(){
    if(glava_fix)system("killall -r glava");

    system("jdsp restart");

    if(glava_fix)system("setsid glava -d >/dev/null 2>&1 &");
}
void MainWindow::DisableFX(){
    //Apply instantly
    if(!lockapply)ConfirmConf();
}

//---Save/Load Presets
void MainWindow::LoadPresetFile(const QString& filename){
    const QString& src = filename;
    const QString dest = QString::fromStdString(path);
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    writeLog("Loading from " + filename+ " (main/loadpreset)");
    reloadConfig();
    ConfirmConf();
}
void MainWindow::SavePresetFile(const QString& filename){
    const QString src = QString::fromStdString(path);
    const QString& dest = filename;
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    writeLog("Saving to " + filename+ " (main/savepreset)");
}
void MainWindow::LoadExternalFile(){
    QString filename = QFileDialog::getOpenFileName(this,"Load custom audio.conf","","*.conf");
    if(filename=="")return;
    const QString& src = filename;
    const QString dest = QString::fromStdString(path);
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    writeLog("Loading from " + filename+ " (main/loadexternal)");
    reloadConfig();
    ConfirmConf();
}
void MainWindow::SaveExternalFile(){
    QString filename = QFileDialog::getSaveFileName(this,"Save current audio.conf","","*.conf");
    if(filename=="")return;
    QFileInfo fi(filename);
    QString ext = fi.suffix();
    if(ext!="conf")filename.append(".conf");

    const QString src = QString::fromStdString(path);
    const QString dest = filename;
    if (QFile::exists(dest))QFile::remove(dest);

    QFile::copy(src,dest);
    writeLog("Saving to " + filename+ " (main/saveexternal)");
}

//---UI Config Loader
void MainWindow::decodeAppConfig(const string& key,const string& value){
    //cout << "AppConf: " << key << " -> " << value << endl;
    switch (resolveAppConfig(key)) {
    case unknownApp: {
        writeLog("Unable to resolve key: " + QString::fromStdString(key) + " (main/parser)");
        break;
    }
    case autoapply: {
        autofx = value=="true";
        break;
    }
    case autoapplymode: {
        autofxmode = atoi(value.c_str());
        break;
    }
    case configpath: {
        if(value.size() <= 2) break;
        path = value.substr(1, value.size() - 2);
        break;
    }
    case glavafix: {
        glava_fix = value=="true";
        break;
    }
    case customwhiteicons: {
        custom_whiteicons = value=="true";
        break;
    }
    case borderpadding: {
        bd_padding = value=="true";
        break;
    }
    case stylesheet: {
        style_sheet = value;
        break;
    }
    case thememode: {
        theme_mode = atoi(value.c_str());
        break;
    }
    case colorpalette: {
        color_palette = value;
        break;
    }
    case theme: {
        theme_str = value;
        break;
    }
    case custompalette: {
        custom_palette = value;
        break;
    }
    }
}
void MainWindow::loadAppConfig(bool once){
    writeLog("Reloading UI-Config... (main/uiparser)");
    std::ifstream cFile(appcpath);
    if (cFile.is_open())
    {
        std::string line;
        while(getline(cFile, line)){
            if(line[0] == '#' || line.empty()) continue;
            auto delimiterPos = line.find('=');
            auto name = line.substr(0, delimiterPos);
            auto value = line.substr(delimiterPos + 1);
            decodeAppConfig(name,value);
        }
        cFile.close();
    }
    else {
        writeLog("Couldn't open UI-Config file for reading (main/uiparser))");
        ofstream outfile(appcpath);
        outfile << default_appconfig << endl;
        outfile.close();
        if(!once)loadAppConfig(true);
    }
}

//---UI Config Generator
void MainWindow::SaveAppConfig(){
    bool afx = autofx;
    const string& cpath = path;
    bool g_fix = glava_fix;
    const string &ssheet = style_sheet;
    int tmode = theme_mode;
    const string &cpalette = color_palette;
    const string &custompal = custom_palette;
    bool w_ico = custom_whiteicons;
    int aamode=autofxmode;
    const string &thm=theme_str;
    bool bd=bd_padding;

    string appconfig;
    stringstream converter1;
    converter1 << boolalpha << afx;
    appconfig += "autoapply=" + converter1.str() + "\n";
    appconfig += "autoapplymode=" + to_string(aamode) + "\n";

    appconfig += "configpath=\"" + cpath + "\"\n";

    stringstream converter3;
    converter3 << boolalpha << g_fix;
    appconfig += "glavafix=" + converter3.str() + "\n";

    appconfig += "theme=" + thm + "\n";
    appconfig += "stylesheet=" + ssheet + "\n";
    appconfig += "thememode=" + to_string(tmode) + "\n";

    appconfig += "colorpalette=" + cpalette + "\n";
    stringstream converter4;
    converter4 << boolalpha << w_ico;
    appconfig += "customwhiteicons=" + converter4.str() + "\n";
    appconfig += "custompalette=" + custompal + "\n";

    stringstream converter5;
    converter5 << boolalpha << bd;
    appconfig += "borderpadding=" + converter5.str() + "\n";

    ofstream myfile(appcpath);
    if (myfile.is_open())
    {
        writeLog("Updating UI-Config... (main/uiwriter)");
        myfile << appconfig;
        myfile.close();
    }
    else{
        writeLog("Unable to write to file at '" + QString::fromStdString(appcpath) + "'; attempting to reloading ui-config... (main/uiwriter)");
        loadAppConfig();
    }
}

//---JDSP Config Loader
void MainWindow::reloadConfig(){
    lockapply=true;
    writeLog("Reloading JDSP Config... (main/parser)");
    std::ifstream cFile(path);
    if (cFile.is_open())
    {
        std::string line;
        while(getline(cFile, line)){
            if(QString::fromStdString(line).trimmed()[0] == '#' || line.empty()) continue; //Skip commented lines
            auto delimiterInlineComment = line.find('#'); //Look for config properties mixed up with comments
            auto extractedProperty = line.substr(0, delimiterInlineComment);
            auto delimiterPos = extractedProperty.find('=');
            auto name = extractedProperty.substr(0, delimiterPos);
            auto value = extractedProperty.substr(delimiterPos + 1);
            loadConfig(name,value);
        }
        cFile.close();
    }
    else {
        writeLog("Unable to read file at '" + QString::fromStdString(path) + "'; jdsp config not found (main/parser)");
        QMessageBox msgBox;
        msgBox.setText("JDSP configuration file not found at \n" + QString::fromStdString(path) + "");
        msgBox.setInformativeText("Make sure JDSP is properly installed and an audio.conf is existing in the config directory.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
    }
    lockapply=false;
}
void MainWindow::loadConfig(const string& key,string value){
    //cout << key << " -> " << value << endl;

    if(value.empty()||is_only_ascii_whitespace(value)){
        mainwin->writeLog("Key " + QString::fromStdString(key) + " is empty (main/parser)");
        return;
    }
    int i = -1;
    if(is_number(value))i = stoi(value);
    switch (resolveConfig(key)) {
    case enable: {
        if(value=="true") ui->disableFX->setChecked(false);
        else ui->disableFX->setChecked(true);
        break;
    }
    case analogmodelling_enable: {
        ui->analog->setChecked(value=="true");
        break;
    }
    case analogmodelling_tubedrive: {
        ui->analog_tubedrive->setValue(std::stoi(value));
        update(i,ui->analog_tubedrive);
        break;
    }
    case bass_enable: {
        ui->bassboost->setChecked(value=="true");
        break;
    }
    case bass_mode: {
        ui->bassstrength->setValue(std::stoi(value));
        update(i,ui->bassstrength);
        break;
    }
    case bass_filtertype: {
        ui->bassfiltertype->setCurrentIndex(std::stoi(value));
        update(i,ui->bassfiltertype);
        break;
    }
    case bass_freq: {
        ui->bassfreq->setValue(std::stoi(value));
        update(i,ui->bassfreq);
        break;
    }
    case headset_enable: {
        ui->reverb->setChecked(value=="true");
        break;
    }
    case headset_preset: {
        ui->reverb_strength->setValue(std::stoi(value));
        update(i,ui->reverb_strength);
        break;
    }
    case stereowide_enable: {
        ui->stereowidener->setChecked(value=="true");
        break;
    }
    case stereowide_mode: {
        ui->stereowidener_lvl->setValue(std::stoi(value));
        update(i,ui->stereowidener_lvl);
        break;
    }
    case bs2b_enable: {
        ui->bs2b->setChecked(value=="true");
        break;
    }
    case bs2b_mode: {
        ui->bs2b_preset_cb->setCurrentIndex(std::stoi(value));
        update(i,ui->bs2b_preset_cb);
        break;
    }
    case compression_enable: {
        ui->enable_comp->setChecked(value=="true");
        break;
    }
    case compression_pregain: {
        ui->comp_pregain->setValue(std::stoi(value));
        update(i,ui->comp_pregain);
        break;
    }
    case compression_threshold: {
        ui->comp_thres->setValue(std::stoi(value));
        update(i,ui->comp_thres);
        break;
    }
    case compression_knee: {
        ui->comp_knee->setValue(std::stoi(value));
        update(i,ui->comp_knee);
        break;
    }
    case compression_ratio: {
        ui->comp_ratio->setValue(std::stoi(value));
        update(i,ui->comp_ratio);
        break;
    }
    case compression_attack: {
        ui->comp_attack->setValue(std::stoi(value));
        update(i,ui->comp_attack);
        break;
    }
    case compression_release: {
        ui->comp_release->setValue(std::stoi(value));
        update(i,ui->comp_release);
        break;
    }
    case tone_enable: {
        ui->enable_eq->setChecked(value=="true");
        break;
    }
    case tone_filtertype: {
        ui->eqfiltertype->setCurrentIndex(std::stoi(value));
        update(i,ui->eqfiltertype);
        break;
    }
    case tone_eq: {
        char* eq_tone = (char*)value.c_str();
        char* eq_str = strtok(eq_tone, ";");
        int count = 0;
        while (eq_str != nullptr) {
            QSlider *sld;
            if (count == 0)sld = ui->eq1;
            else if (count == 1)sld = ui->eq2;
            else if (count == 2)sld = ui->eq3;
            else if (count == 3)sld = ui->eq4;
            else if (count == 4)sld = ui->eq5;
            else if (count == 5)sld = ui->eq6;
            else if (count == 6)sld = ui->eq7;
            else if (count == 7)sld = ui->eq8;
            else if (count == 8)sld = ui->eq9;
            else if (count == 9)sld = ui->eq10;
            else if (count == 10)sld = ui->eq11;
            else if (count == 11)sld = ui->eq12;
            else if (count == 12)sld = ui->eq13;
            else if (count == 13)sld = ui->eq14;
            else if (count == 14)sld = ui->eq15;
            else break;

            QString qEq(eq_str);
            if(count == 0)qEq.remove(0,1);
            else if(count == 14) qEq.chop(1);
            sld->setValue(qEq.toInt());

            eq_str = strtok (nullptr, ";");
            count++;
        }
        break;
    }
    case masterswitch_limthreshold: {
        ui->limthreshold->setValue(std::stoi(value));
        update(i,ui->limthreshold);
        break;
    }
    case masterswitch_limrelease: {
        ui->limrelease->setValue(std::stoi(value));
        update(i,ui->limrelease);
        break;
    }
    case ddc_enable: {
        ui->ddc_enable->setChecked(value=="true");
        break;
    }
    case ddc_file: {
        if(value.size() <= 2) break;
        value = value.substr(1, value.size() - 2);
        QString ddc_in= QString::fromStdString(value);
        ddcpath = ddc_in;
        break;
    }
    case unknown: {
        writeLog("Unable to resolve key: " + QString::fromStdString(key)+ " (main/uiparser)");
        break;
    }
    }
}

//---JDSP Config Generator
string MainWindow::getMisc(){
    string out;
    string n = "\n";
    string esc = "\"";

    QString tubedrive = QString::number(ui->analog_tubedrive->value());
    QString limthres = QString::number(ui->limthreshold->value());
    QString limrel = QString::number(ui->limrelease->value());

    out += "analogmodelling_enable=";
    if(ui->analog->isChecked())out += "true" + n;
    else out += "false" + n;
    out += "analogmodelling_tubedrive=";
    out += tubedrive.toUtf8().constData() + n;

    out += "masterswitch_limthreshold=";
    out += limthres.toUtf8().constData() + n;
    out += "masterswitch_limrelease=";
    out += limrel.toUtf8().constData() + n;

    out += "ddc_enable=";
    if(ui->ddc_enable->isChecked())out += "true" + n;
    else out += "false" + n;

    if(ddcpath!=""){
        out += "ddc_file=" + esc + ddcpath.toUtf8().constData() + esc + n;
    }else{
        out += "ddc_file=\"none\"" + n;
    }

    return out;
}
string MainWindow::getComp() {
    string out;
    string n = "\n";
    QString pregain = QString::number(ui->comp_pregain->value());
    QString knee = QString::number(ui->comp_knee->value());
    QString ratio = QString::number(ui->comp_ratio->value());
    QString thres = QString::number(ui->comp_thres->value());
    QString atk = QString::number(ui->comp_attack->value());
    QString rel = QString::number(ui->comp_release->value());

    out += "compression_enable=";
    if(ui->enable_comp->isChecked())out += "true" + n;
    else out += "false" + n;

    out += "compression_threshold=";
    out += thres.toUtf8().constData() + n;
    out += "compression_pregain=";
    out += pregain.toUtf8().constData() + n;
    out += "compression_knee=";
    out += knee.toUtf8().constData() + n;
    out += "compression_ratio=";
    out += ratio.toUtf8().constData() + n;
    out += "compression_attack=";
    out += atk.toUtf8().constData() + n;
    out += "compression_release=";
    out += rel.toUtf8().constData() + n;

    return out;
}
string MainWindow::getEQ() {
    string out;
    string n = "\n";
    string s = ";";

    QString eq1 = QString::number(ui->eq1->value());
    QString eq2 = QString::number(ui->eq2->value());
    QString eq3 = QString::number(ui->eq3->value());
    QString eq4 = QString::number(ui->eq4->value());
    QString eq5 = QString::number(ui->eq5->value());
    QString eq6 = QString::number(ui->eq6->value());
    QString eq7 = QString::number(ui->eq7->value());
    QString eq8 = QString::number(ui->eq8->value());
    QString eq9 = QString::number(ui->eq9->value());
    QString eq10 = QString::number(ui->eq10->value());
    QString eq11 = QString::number(ui->eq11->value());
    QString eq12 = QString::number(ui->eq12->value());
    QString eq13 = QString::number(ui->eq13->value());
    QString eq14 = QString::number(ui->eq14->value());
    QString eq15 = QString::number(ui->eq15->value());
    out += "tone_enable=";
    if(ui->enable_eq->isChecked())out += "true" + n;
    else out += "false" + n;
    out += "tone_filtertype=";
    out += to_string(ui->eqfiltertype->currentIndex()) + n;

    out += "tone_eq=\"";
    out += eq1.toUtf8().constData() + s;
    out += eq2.toUtf8().constData() + s;
    out += eq3.toUtf8().constData() + s;
    out += eq4.toUtf8().constData() + s;
    out += eq5.toUtf8().constData() + s;
    out += eq6.toUtf8().constData() + s;
    out += eq7.toUtf8().constData() + s;
    out += eq8.toUtf8().constData() + s;
    out += eq9.toUtf8().constData() + s;
    out += eq10.toUtf8().constData() + s;
    out += eq11.toUtf8().constData() + s;
    out += eq12.toUtf8().constData() + s;
    out += eq13.toUtf8().constData() + s;
    out += eq14.toUtf8().constData() + s;
    out += eq15.toUtf8().constData();
    out += "\"" + n;

    return out;
}
string MainWindow::getBass() {
    string out;
    string n = "\n";
    QString bassfreq = QString::number(ui->bassfreq->value());
    QString bassmode = QString::number(ui->bassstrength->value());
    QString bassfilter = QString::number(ui->bassfiltertype->currentIndex());

    out += "bass_enable=";
    if(ui->bassboost->isChecked())out += "true" + n;
    else out += "false" + n;
    out += "bass_mode=";
    out += bassmode.toUtf8().constData() + n;
    out += "bass_filtertype=";
    out += bassfilter.toUtf8().constData() + n;
    out += "bass_freq=";
    out += bassfreq.toUtf8().constData() + n;
    return out;
}
string MainWindow::getSurround() {
    string out;
    string n = "\n";
    out += "stereowide_enable=";
    if(ui->stereowidener->isChecked())out += "true" + n;
    else out += "false" + n;
    out += "stereowide_mode=";
    out += to_string(ui->stereowidener_lvl->value()) + n;

    out += "bs2b_enable=";
    if(ui->bs2b->isChecked())out += "true" + n;
    else out += "false" + n;
    out += "bs2b_mode=";
    out += to_string(ui->bs2b_preset_cb->currentIndex()) + n;

    out += "headset_enable=";
    if(ui->reverb->isChecked())out += "true" + n;
    else out += "false" + n;
    out += "headset_preset=";
    out += to_string(ui->reverb_strength->value()) + n;

    return out;
}

//---Presets
void MainWindow::setEQ(const int* data){
    lockapply=true;
    ui->eq1->setValue(data[0]);
    ui->eq2->setValue(data[1]);
    ui->eq3->setValue(data[2]);
    ui->eq4->setValue(data[3]);
    ui->eq5->setValue(data[4]);
    ui->eq6->setValue(data[5]);
    ui->eq7->setValue(data[6]);
    ui->eq8->setValue(data[7]);
    ui->eq9->setValue(data[8]);
    ui->eq10->setValue(data[9]);
    ui->eq11->setValue(data[10]);
    ui->eq12->setValue(data[11]);
    ui->eq13->setValue(data[12]);
    ui->eq14->setValue(data[13]);
    ui->eq15->setValue(data[14]);
    lockapply=false;
    OnUpdate(true);
}
void MainWindow::setRoompreset(int data){
    lockapply=true;
    ui->reverb_strength->setValue(data);
    lockapply=false;
    OnUpdate(true);
}
void MainWindow::updateroompreset(){
    QString selection = ui->roompresets->currentText();
    if(selection == "Small room")setRoompreset(8);
    else if(selection == "Medium room")setRoompreset(10);
    else if(selection == "Large room")setRoompreset(12);
    else if(selection == "Plate high")setRoompreset(15);
    else if(selection == "Plate low")setRoompreset(16);
    else if(selection == "Long reverb 1")setRoompreset(17);
    else if(selection == "Long reverb 2")setRoompreset(18);
    else setRoompreset(0);
}
void MainWindow::ResetEQ(){
    ui->eq1->setValue(0);
    ui->eq2->setValue(0);
    ui->eq3->setValue(0);;
    ui->eq4->setValue(0);
    ui->eq5->setValue(0);
    ui->eq6->setValue(0);
    ui->eq7->setValue(0);
    ui->eq8->setValue(0);
    ui->eq9->setValue(0);
    ui->eq10->setValue(0);
    ui->eq11->setValue(0);
    ui->eq12->setValue(0);
    ui->eq13->setValue(0);
    ui->eq14->setValue(0);
    ui->eq15->setValue(0);
}
void MainWindow::CopyEQ(){
    string s = to_string(ui->eq1->value()) + "," + to_string(ui->eq2->value()) + "," + to_string(ui->eq3->value()) + "," + to_string(ui->eq4->value()) + "," + to_string(ui->eq5->value()) + ",";
    s += to_string(ui->eq6->value()) + "," + to_string(ui->eq7->value()) + "," + to_string(ui->eq8->value()) + "," + to_string(ui->eq9->value()) + "," + to_string(ui->eq10->value()) + ",";
    s += to_string(ui->eq11->value()) + "," + to_string(ui->eq12->value()) + "," + to_string(ui->eq13->value()) + "," + to_string(ui->eq14->value()) + "," + to_string(ui->eq15->value());

    QClipboard* a = app->clipboard();
    a->setText(QString::fromStdString(s));
}
void MainWindow::PasteEQ(){
    QClipboard* a = app->clipboard();
    std::string str = a->text().toUtf8().constData();
    std::vector<int> vect;
    std::stringstream ss(str);
    int i;
    while (ss >> i)
    {
        vect.push_back(i);
        if (ss.peek() == ',')
            ss.ignore();
    }
    int data[100];
    std::copy(vect.begin(), vect.end(), data);
    setEQ(data);
}


//---Updates Unit-Label
void MainWindow::update(int d,QObject *alt){
    if(lockapply&&alt==nullptr)return;//Skip if lockapply-flag is set (when setting presets, ...)
    QObject* obj;

    if(alt==nullptr)obj = sender();
    else obj = alt;

    QString pre = "";
    QString post = "";
    if(obj==ui->bassstrength){
        if(d <= 200) updateWidgetUnit(obj,"A bit (" + QString::number( d ) + ")",alt==nullptr);
        else if(d <= 360) updateWidgetUnit(obj,"Slight (" + QString::number( d )+")",alt==nullptr);
        else if(d <= 600) updateWidgetUnit(obj,"Moderate (" + QString::number( d )+")",alt==nullptr);
        else if(d <= 900) updateWidgetUnit(obj,"Moderate (" + QString::number( d )+")",alt==nullptr);
        else updateWidgetUnit(obj,"Extreme (" + QString::number( d )+")",alt==nullptr);
    }
    else if(obj==ui->stereowidener_lvl){
        if(d==0) updateWidgetUnit(obj,"A bit",alt==nullptr);
        else if(d==1) updateWidgetUnit(obj,"Slight",alt==nullptr);
        else if(d==2) updateWidgetUnit(obj,"Moderate",alt==nullptr);
        else if(d==3) updateWidgetUnit(obj,"High",alt==nullptr);
        else if(d==4) updateWidgetUnit(obj,"Super",alt==nullptr);
        else updateWidgetUnit(obj,"Mode "+QString::number( d ),alt==nullptr);
    }
    else if(obj==ui->comp_ratio){
        updateWidgetUnit(obj,"1:"+QString::number( d ),alt==nullptr);
    }
    else if(obj==ui->eq1||obj==ui->eq2||obj==ui->eq3||obj==ui->eq4||obj==ui->eq5
            ||obj==ui->eq6||obj==ui->eq7||obj==ui->eq8||obj==ui->eq9||obj==ui->eq10
            ||obj==ui->eq11||obj==ui->eq12||obj==ui->eq13||obj==ui->eq14||obj==ui->eq15){
        updateeq(d,obj);
    }
    else if(obj==ui->bassfiltertype||obj==ui->eqfiltertype||obj==ui->bs2b_preset_cb){
        // Ignore these UI elements
    }
    else{
        if(obj==ui->comp_thres||obj==ui->comp_pregain||obj==ui->comp_knee||obj==ui->limthreshold)post = "dB";
        else if(obj==ui->comp_attack||obj==ui->comp_release||obj==ui->limrelease)post = "ms";
        else if(obj==ui->bassfreq)post = "Hz";
        updateWidgetUnit(obj,pre + QString::number(d) + post,alt==nullptr);
    }
    if(!lockapply||obj!=nullptr)OnUpdate(false);
}
void MainWindow::updateeq(int f,QObject *obj){
    QString pre;

    if (obj == ui->eq1)pre="25Hz";
    else if (obj == ui->eq2)pre="40Hz";
    else if (obj == ui->eq3)pre="63Hz";
    else if (obj == ui->eq4)pre="100Hz";
    else if (obj == ui->eq5)pre="160Hz";
    else if (obj == ui->eq6)pre="250Hz";
    else if (obj == ui->eq7)pre="400Hz";
    else if (obj == ui->eq8)pre="630Hz";
    else if (obj == ui->eq9)pre="1kHz";
    else if (obj == ui->eq10)pre="1.6kHz";
    else if (obj == ui->eq11)pre="2.5kHz";
    else if (obj == ui->eq12)pre="4kHz";
    else if (obj == ui->eq13)pre="6.3kHz";
    else if (obj == ui->eq14)pre="10kHz";
    else if (obj == ui->eq15)pre="16kHz";

    pre += ": ";
    if(f < 0 ) pre += "-";
    
    QString s;
    if(to_string(abs(f)%100).length()==1)
    {
        char buffer[5];
        snprintf(buffer, sizeof(buffer), "%02d", abs(f)%100);
        s = pre + QString::number(abs(f)/100) + "."  + QString::fromUtf8(buffer) + "dB";
    }
    else{
        s = pre + QString::number(abs(f)/100) + "."  + QString::number(abs(f%100)) + "dB";
    }
    ui->info->setText(s);
}
float MainWindow::translate(int value,int leftMin,int leftMax,float rightMin,float rightMax){
    float leftSpan = leftMax - leftMin;
    float rightSpan = rightMax - rightMin;
    float valueScaled = float(value - leftMin) / float(leftSpan);
    return rightMin + (valueScaled * rightSpan);
}
void MainWindow::updateWidgetUnit(QObject* sender,QString text,bool viasignal){
    QWidget *w = qobject_cast<QWidget*>(sender);
    w->setToolTip(text);
    if(viasignal)ui->info->setText(text);
}

//---Getter/Setter
bool MainWindow::getAutoFx(){
    return autofx;
}
void MainWindow::setAutoFx(bool afx){
    autofx = afx;
    SaveAppConfig();
}
void MainWindow::setGFix(bool f){
    glava_fix = f;
    SaveAppConfig();
}
bool MainWindow::getGFix(){
    return glava_fix;
}
void MainWindow::setPath(string npath){
    path = std::move(npath);
    SaveAppConfig();
}
string MainWindow::getPath(){
    return path;
}
void MainWindow::setStylesheet(string s){
    style_sheet = std::move(s);
    SetStyle();
    SaveAppConfig();
}
string MainWindow::getStylesheet(){
    return style_sheet;
}
int MainWindow::getThememode(){
    return theme_mode;
}
void MainWindow::setThememode(int mode){
    //Modes:
    //  0 - Default/QSS
    //  1 - Color Palette
    theme_mode = mode;
    SetStyle();
    SaveAppConfig();
}
void MainWindow::setColorpalette(string s){
    color_palette = std::move(s);
    SetStyle();
    SaveAppConfig();
}
string MainWindow::getColorpalette(){
    return color_palette;
}
void MainWindow::setCustompalette(string s){
    custom_palette = std::move(s);
    SetStyle();
    SaveAppConfig();
}
string MainWindow::getCustompalette(){
    return custom_palette;
}
void MainWindow::setWhiteIcons(bool b){
    custom_whiteicons = b;
    SetStyle();
    SaveAppConfig();
}
bool MainWindow::getWhiteIcons(){
    return custom_whiteicons;
}
int MainWindow::getAutoFxMode(){
    return autofxmode;
}
void MainWindow::setAutoFxMode(int mode){
    autofxmode = mode;
    SaveAppConfig();
}
void MainWindow::setTheme(string thm){
    theme_str = std::move(thm);
    SetStyle();
    SaveAppConfig();
}
string MainWindow::getTheme(){
    return theme_str;
}
void MainWindow::setBorderPadding(bool b){
    bd_padding = b;
    SetStyle();
    SaveAppConfig();
}
bool MainWindow::getBorderPadding(){
    return bd_padding;
}


//---Connect UI-Signals
void MainWindow::ConnectActions(){
    connect(ui->apply, SIGNAL(clicked()), this, SLOT(ConfirmConf()));
    connect(ui->disableFX, SIGNAL(clicked()), this, SLOT(DisableFX()));
    connect(ui->reset, SIGNAL(clicked()), this, SLOT(Reset()));
    connect(ui->cpreset, SIGNAL(clicked()), this, SLOT(OpenPreset()));
    connect(ui->set, SIGNAL(clicked()), this, SLOT(OpenSettings()));

    connect(ui->reseteq, SIGNAL(clicked()), this, SLOT(ResetEQ()));

    connect(ui->set, SIGNAL(clicked()), this, SLOT(OpenSettings()));
    connect(ui->ddcTable->selectionModel(),SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),this,SLOT(updateDDC(const QItemSelection &, const QItemSelection &)));

    connect( ui->reverb_strength , SIGNAL(valueChanged(int)),this,  SLOT(update(int)));
    connect( ui->analog_tubedrive , SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect( ui->stereowidener_lvl , SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->bassfreq, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->bassstrength, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->limthreshold, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->limrelease, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->comp_release, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->comp_pregain, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->comp_knee, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->comp_ratio, SIGNAL(valueChanged(int)),this,SLOT(update(int)));
    connect(ui->comp_thres, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->comp_attack, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->comp_release, SIGNAL(valueChanged(int)),this, SLOT(update(int)));

    connect(ui->eq1, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq2, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq3, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq4, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq5, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq6, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq7, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq8, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq9, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq10, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq11, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq12, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq13, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq14, SIGNAL(valueChanged(int)),this, SLOT(update(int)));
    connect(ui->eq15, SIGNAL(valueChanged(int)),this, SLOT(update(int)));

    connect( ui->bassboost , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->bs2b , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->stereowidener, SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->analog , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->reverb , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->enable_eq , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->enable_comp , SIGNAL(clicked()),this, SLOT(OnUpdate()));
    connect( ui->ddc_enable , SIGNAL(clicked()),this, SLOT(OnUpdate()));

    connect(ui->bs2b_preset_cb, SIGNAL(currentIndexChanged(int)),this,SLOT(OnUpdate()));
    connect(ui->eqfiltertype, SIGNAL(currentIndexChanged(int)),this,SLOT(OnUpdate()));
    connect(ui->bassfiltertype, SIGNAL(currentIndexChanged(int)),this,SLOT(OnUpdate()));
    connect(ui->roompresets, SIGNAL(currentIndexChanged(int)),this,SLOT(updateroompreset()));

    connect( ui->reverb_strength, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->stereowidener_lvl , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->bassfreq , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->bassstrength , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->analog_tubedrive , SIGNAL(sliderReleased()),this,  SLOT(OnRelease()));
    connect( ui->limthreshold , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect( ui->limrelease , SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->comp_release, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->comp_pregain, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->comp_knee, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->comp_ratio, SIGNAL(sliderReleased()),this,SLOT(OnRelease()));
    connect(ui->comp_thres, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->comp_attack, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));

    connect(ui->eq1, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq2, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq3, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq4, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq5, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq6, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq7, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq8, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq9, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq10, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq11, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq12, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq13, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq14, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));
    connect(ui->eq15, SIGNAL(sliderReleased()),this, SLOT(OnRelease()));

    connect(ui->ddc_reload, SIGNAL(clicked()), this, SLOT(reloadDDC()));
    connect(ui->ddc_files, SIGNAL(itemSelectionChanged()), this, SLOT(updateDDC_file()));
    connect(ui->ddc_select, SIGNAL(clicked()), this, SLOT(selectDDCFolder()));

}
void MainWindow::reloadDDC(){
    lockddcupdate=true;
    QDir path(ui->ddc_dirpath->text());
    QStringList nameFilter("*.vdc");
    nameFilter.append("*.ddc");
    QStringList files = path.entryList(nameFilter);
    ui->ddc_files->clear();
    if(files.empty()){
        QFont font;
        font.setItalic(true);
        font.setPointSize(11);

        QListWidgetItem* placeholder = new QListWidgetItem;
        placeholder->setFont(font);
        placeholder->setText("No VDC files found");
        placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsEnabled);
        ui->ddc_files->addItem(placeholder);
    }
    else ui->ddc_files->addItems(files);
    lockddcupdate=false;
}
void MainWindow::updateDDC_file(){
    if(lockddcupdate || ui->ddc_files->selectedItems().count()<1)return; //Clearing Selection by code != User Interaction
    QString path = QDir(ui->ddc_dirpath->text()).filePath(ui->ddc_files->selectedItems().first()->text());
    if(QFileInfo::exists(path) && QFileInfo(path).isFile())ddcpath=path;
    OnUpdate();
}
void MainWindow::selectDDCFolder(){

    QFileDialog *fd = new QFileDialog;
    fd->setFileMode(QFileDialog::Directory);
    fd->setOption(QFileDialog::ShowDirsOnly);
    fd->setViewMode(QFileDialog::Detail);
    QString result = fd->getExistingDirectory();
    if (result!="")
    {
        ui->ddc_dirpath->setText(result);
        reloadDDC();
    }
}
//---Helper
bool MainWindow::is_number(const string& s)
{
    return !s.empty() && std::find_if(s.begin(),
                                      s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}
bool MainWindow::is_only_ascii_whitespace( const std::string& str )
{
    auto it = str.begin();
    do {
        if (it == str.end()) return true;
    } while (*it >= 0 && *it <= 0x7f && std::isspace(*(it++)));
    return false;
}
