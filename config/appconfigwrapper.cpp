#include "appconfigwrapper.h"

#include <QDir>
#include <QDebug>

using namespace std;

AppConfigWrapper::AppConfigWrapper(StyleHelper* stylehelper)
{
    appconf = new ConfigContainer();
    m_stylehelper = stylehelper;
}
void AppConfigWrapper::saveAppConfig(){
    auto file = QString("%1/.config/jamesdsp/ui.2.conf").arg(QDir::homePath());
    ConfigIO::writeFile(file,appconf->getConfigMap());
}
void AppConfigWrapper::loadAppConfig(){
    auto map = ConfigIO::readFile(QString("%1/.config/jamesdsp/ui.2.conf").arg(QDir::homePath()));
    appconf->setConfigMap(map);
}

bool AppConfigWrapper::getAutoFx(){
    return appconf->getBool("apply.auto.enable",true,true);
}
void AppConfigWrapper::setAutoFx(bool afx){
    appconf->setValue("apply.auto.enable",QVariant(afx));
    saveAppConfig();
}
bool AppConfigWrapper::getMuteOnRestart(){
    return appconf->getBool("apply.mutedreload");
}
void AppConfigWrapper::setMuteOnRestart(bool on){
    appconf->setValue("apply.mutedreload",QVariant(on));
    saveAppConfig();
}
void AppConfigWrapper::setGFix(bool f){
    appconf->setValue("apply.fixglava",QVariant(f));
    saveAppConfig();
}
bool AppConfigWrapper::getGFix(){
    return appconf->getBool("apply.fixglava");
}
void AppConfigWrapper::setLiveprogAutoExtract(bool f){
    appconf->setValue("liveprog.default.autoextract",QVariant(f));
    saveAppConfig();
}
bool AppConfigWrapper::getLiveprogAutoExtract(){
    return appconf->getBool("liveprog.default.autoextract",true,true);
}
void AppConfigWrapper::setPath(const QString& npath){
    appconf->setValue("io.configpath",QVariant(QString("\"%1\"").arg(npath)));
    saveAppConfig();
}
QString AppConfigWrapper::getPath(){
    QString path = "";
    path = appconf->getString("io.configpath",true);
    path = chopFirstLastChar(path);
    if(path.length() < 2)
        return QString("%1/.config/jamesdsp/audio.conf").arg(QDir::homePath());
    return path;
}
void AppConfigWrapper::setStylesheet(const QString& s){
    appconf->setValue("theme.stylesheet",QVariant(s));
    m_stylehelper->SetStyle();
    emit styleChanged();
    saveAppConfig();
}
QString AppConfigWrapper::getStylesheet(){
    return appconf->getString("theme.stylesheet");
}
int AppConfigWrapper::getThememode(){
    return appconf->getInt("theme.mode",true,1);
}
void AppConfigWrapper::setThememode(int mode){
    appconf->setValue("theme.mode",QVariant(mode));
    m_stylehelper->SetStyle();
    emit styleChanged();
    saveAppConfig();
}
void AppConfigWrapper::setColorpalette(const QString& s){
    appconf->setValue("theme.palette",QVariant(s));
    m_stylehelper->SetStyle();
    emit styleChanged();
    saveAppConfig();
}
QString AppConfigWrapper::getColorpalette(){
    return appconf->getString("theme.palette");
}
void AppConfigWrapper::setCustompalette(const QString& s){
    appconf->setValue("theme.palette.custom",QVariant(s));
    m_stylehelper->SetStyle();
    emit styleChanged();
    saveAppConfig();
}
QString AppConfigWrapper::getCustompalette(){
    return appconf->getString("theme.palette.custom");
}
void AppConfigWrapper::setWhiteIcons(bool b){
    appconf->setValue("theme.icons.white",QVariant(b));
    m_stylehelper->SetStyle();
    emit styleChanged();
    saveAppConfig();
}
bool AppConfigWrapper::getWhiteIcons(){
    return appconf->getBool("theme.icons.white");
}
int AppConfigWrapper::getAutoFxMode(){
    return appconf->getInt("apply.auto.mode",true,1);
}
void AppConfigWrapper::setAutoFxMode(int mode){
    appconf->setValue("apply.auto.mode",QVariant(mode));
    saveAppConfig();
}
ReloadMethod AppConfigWrapper::getReloadMethod(){
    return (ReloadMethod)appconf->getInt("apply.method");
}
void AppConfigWrapper::setReloadMethod(ReloadMethod mode){
    appconf->setValue("apply.method",QVariant((uint)mode));
    saveAppConfig();
}
void AppConfigWrapper::setIrsPath(const QString& npath){
    appconf->setValue("convolver.default.irspath",QVariant(QString("\"%1\"").arg(npath)));
    saveAppConfig();
}
void AppConfigWrapper::setTheme(const QString& thm){
    appconf->setValue("theme.name",QVariant(thm));
    m_stylehelper->SetStyle();
    emit styleChanged();
    saveAppConfig();
}
QString AppConfigWrapper::getTheme(){
    QString name = appconf->getString("theme.name");
    if(name.isEmpty()){
        name = "Fusion";
        appconf->setValue("theme.name",name);
        emit styleChanged();
    }
    return name;
}
QString AppConfigWrapper::getIrsPath(){
    QString irs_path = chopFirstLastChar(appconf->getString("convolver.default.irspath",false));
    if(irs_path.length() < 2)
        return QString("%1/IRS").arg(QDir::homePath());
    return irs_path;
}
void AppConfigWrapper::setDDCPath(const QString& npath){
    appconf->setValue("ddc.default.path",QVariant(QString("\"%1\"").arg(npath)));
    saveAppConfig();
}
QString AppConfigWrapper::getDDCPath(){
    QString irs_path = chopFirstLastChar(appconf->getString("ddc.default.path",false));
    if(irs_path.length() < 2)
        return QString("%1/DDC").arg(QDir::homePath());
    return irs_path;
}
void AppConfigWrapper::setLiveprogPath(const QString& npath){
    appconf->setValue("liveprog.default.path",QVariant(QString("\"%1\"").arg(npath)));
    saveAppConfig();
}
QString AppConfigWrapper::getLiveprogPath(){
    QString absolute = QFileInfo(getPath()).absoluteDir().absolutePath();
    QString lp_path = chopFirstLastChar(appconf->getString("liveprog.default.path",false));
    if(lp_path.length() < 2){
        QDir(absolute).mkdir("liveprog");
        QString defaultstr = QString("%1/liveprog").arg(absolute);
        setLiveprogPath(defaultstr);
        return defaultstr;
    }
    return lp_path;
}
int AppConfigWrapper::getTrayMode(){
    return appconf->getInt("session.tray.mode");
}
void AppConfigWrapper::setTrayMode(int mode){
    appconf->setValue("session.tray.mode",QVariant(mode));
    saveAppConfig();
}
void AppConfigWrapper::setTrayContextMenu(const QString& ctx){
    appconf->setValue("session.tray.contextmenu",QVariant(ctx));
    saveAppConfig();
}
QString AppConfigWrapper::getTrayContextMenu(){
    QString name = appconf->getString("session.tray.contextmenu");
    return name;
}
void AppConfigWrapper::setSpectrumEnable(bool b){
    appconf->setValue("visualizer.spectrum.enable",QVariant(b));
    emit spectrumChanged();
    saveAppConfig();
}
bool AppConfigWrapper::getSpetrumEnable(){
    return appconf->getBool("visualizer.spectrum.enable");
}
int AppConfigWrapper::getSpectrumBands(){
    return appconf->getInt("visualizer.spectrum.bands");
}
void AppConfigWrapper::setSpectrumBands(int number){
    appconf->setValue("visualizer.spectrum.bands",QVariant(number));
    emit spectrumChanged();
    saveAppConfig();
}
int AppConfigWrapper::getSpectrumMinFreq(){
    return appconf->getInt("visualizer.spectrum.frequency.min");
}
void AppConfigWrapper::setSpectrumMinFreq(int number){
    appconf->setValue("visualizer.spectrum.frequency.min",QVariant(number));
    emit spectrumChanged();
    saveAppConfig();
}
int AppConfigWrapper::getSpectrumMaxFreq(){
    return appconf->getInt("visualizer.spectrum.frequency.max");
}
void AppConfigWrapper::setSpectrumMaxFreq(int number){
    appconf->setValue("visualizer.spectrum.frequency.max",QVariant(number));
    emit spectrumChanged();
    saveAppConfig();
}
void AppConfigWrapper::setSpectrumGrid(bool b){
    appconf->setValue("visualizer.spectrum.grid",QVariant(b));
    emit spectrumChanged();
    saveAppConfig();
}
bool AppConfigWrapper::getSpetrumGrid(){
    return appconf->getBool("visualizer.spectrum.grid");
}
int AppConfigWrapper::getSpectrumTheme(){
    return appconf->getInt("visualizer.spectrum.theme",true,1);
}
void AppConfigWrapper::setSpectrumTheme(int number){
    appconf->setValue("visualizer.spectrum.theme",QVariant(number));
    emit spectrumChanged();
    saveAppConfig();
}
int AppConfigWrapper::getSpectrumShape(){
    return appconf->getInt("visualizer.spectrum.shape");
}
void AppConfigWrapper::setSpectrumShape(int number){
    appconf->setValue("visualizer.spectrum.shape",QVariant(number));
    emit spectrumChanged();
    saveAppConfig();
}
void AppConfigWrapper::setSpectrumInput(const QString& npath){
    appconf->setValue("visualizer.spectrum.device",QVariant(npath));
    emit spectrumReloadRequired();
    saveAppConfig();
}
QString AppConfigWrapper::getSpectrumInput(){
    QString in = appconf->getString("visualizer.spectrum.device",true);
    if(in.length() < 1){
        QString defaultstr = "jdsp.monitor";
        setSpectrumInput(defaultstr);
        return defaultstr;
    }
    return in;
}
int AppConfigWrapper::getSpectrumRefresh(){
    return appconf->getInt("visualizer.spectrum.interval");
}
void AppConfigWrapper::setSpectrumRefresh(int number){
    appconf->setValue("visualizer.spectrum.interval",QVariant(number));
    emit spectrumReloadRequired();
    saveAppConfig();
}
float AppConfigWrapper::getSpectrumMultiplier(){
    return appconf->getFloat("visualizer.spectrum.multiplier");
}
void AppConfigWrapper::setSpectrumMultiplier(float number){
    appconf->setValue("visualizer.spectrum.multiplier",QVariant(number));
    emit spectrumChanged();
    saveAppConfig();
}
void AppConfigWrapper::setEqualizerPermanentHandles(bool b){
    appconf->setValue("equalizer.handle.permanent",QVariant(b));
    emit eqChanged();
    saveAppConfig();
}
bool AppConfigWrapper::getEqualizerPermanentHandles(){
    return appconf->getBool("equalizer.handle.permanent");
}
void AppConfigWrapper::setIntroShown(bool b){
    appconf->setValue("app.firstlaunch",QVariant(b));
    saveAppConfig();
}
bool AppConfigWrapper::getIntroShown(){
    return appconf->getBool("app.firstlaunch");
}
void AppConfigWrapper::setLegacyTabs(bool b){
    appconf->setValue("theme.tab.legacy",QVariant(b));
    saveAppConfig();
}
bool AppConfigWrapper::getLegacyTabs(){
    return appconf->getBool("theme.tab.legacy");
}
//--------
QString AppConfigWrapper::getAppConfigFilePath(){
    return QString("%1/.config/jamesdsp/ui.2.conf").arg(QDir::homePath());
}
QString AppConfigWrapper::getGraphicEQConfigFilePath(){
    return pathAppend(findDirOfFile(getPath()),"ui.graphiceq.conf");
}
