#include "container.h"

#include <QDebug>
#include <utility> 

#include <misc/loghelper.h>
ConfigContainer::ConfigContainer()
= default;
void ConfigContainer::setValue(const QString& key,QVariant value){
    map[key] = std::move(value);
}
QVariant ConfigContainer::getVariant(const QString& key, bool silent){
    if(!map.contains(key)){
        if(!silent) {
            LogHelper::debug(QString("Requested key '%1' (variant) not found").arg(key));
        }
        return QVariant();
    }
    return map.find(key).value();
}
QString ConfigContainer::getString(const QString& key,bool setToDefaultIfMissing){
    if(!map.contains(key)){
        LogHelper::debug(QString("Requested key '%1' (string) not found").arg(key));
         if(setToDefaultIfMissing)map[key] = "";
        return "";
    }
    return getVariant(key).toString();
}
int ConfigContainer::getInt(const QString& key,bool setToDefaultIfMissing, int defaults){
    if(!map.contains(key)){
        LogHelper::debug(QString("Requested key '%1' (int) not found").arg(key));
        if(setToDefaultIfMissing)map[key] = defaults;
        return 0;
    }
    return getVariant(key).toInt();
}
float ConfigContainer::getFloat(const QString& key){
    if(!map.contains(key)){
        LogHelper::debug(QString("Requested key '%1' (float) not found").arg(key));
        map[key] = 0.0F;
        return 0.0F;
    }
    return getVariant(key).toFloat();
}
bool ConfigContainer::getBool(const QString& key, bool setToDefaultIfMissing, bool defaults){
    if(!map.contains(key)){
        LogHelper::debug(QString("Requested key '%1' (bool) not found").arg(key));
        if(setToDefaultIfMissing)map[key] = defaults;
        return false;
    }
    return getVariant(key).toBool();
}
QVariantMap ConfigContainer::getConfigMap(){
    return map;
}
void ConfigContainer::setConfigMap(const QVariantMap& newmap){
    map.clear();
    for(const auto& e : newmap.keys())
      map[e] = newmap.value(e);
}
