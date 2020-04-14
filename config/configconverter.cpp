#include "configconverter.h"
#include "misc/loghelper.h"
#include "config/container.h"
#include "config/io.h"
#include "misc/presetprovider.h"

#include <QDomElement>
#include <QFile>

ConversionResult ConfigConverter::fromAndroid(QString path)
{
    ConfigContainer *conf = new ConfigContainer();
    ConversionResult result;

    int errorLine, errorColumn;
    QString errorMsg;
    QFile modelFile(path);
    QDomDocument document;
    if (!document.setContent(&modelFile, &errorMsg, &errorLine, &errorColumn))
    {
        QString error(QObject::tr("Syntax error line %1, column %2:\n%3"));
        error = error
                .arg(errorLine)
                .arg(errorColumn)
                .arg(errorMsg);
        LogHelper::error("Converter (from android): " + error + " (converter/syntaxcheck)");
        result.failed = true;
        result.description = error;
        return result;
    }

    QDomElement root = document.documentElement();
    QDomNodeList key = root.elementsByTagName("boolean");
    for(int i = 0; i < key.count(); i++)
    {
        QDomNode keynode = key.at(i);
        if(keynode.isElement())
        {
            QDomElement att = keynode.toElement();
            QString key = att.attribute("name");

            if(cc_blacklistedKeys.contains(key))
                continue;

            if(key == "dsp.ddc.enable")
                result.uses_ddc = true;
            else if(key == "dsp.convolver.enable")
                result.uses_irs = true;
            else if(key == "dsp.liveprog.enable")
                result.uses_liveprog = true;
            else if(key == "dsp.headphone.enable")
                conf->setValue("headset_enable",att.attribute("value") == "true");
            else{
                key.remove("dsp.");
                key.replace(".","_");
                conf->setValue(key,att.attribute("value") == "true");
            }
        }
    }

    QDomNodeList key_str = root.elementsByTagName("string");
    for(int i = 0; i < key_str.count(); i++)
    {
        QDomNode keynode = key_str.at(i);
        if(keynode.isElement())
        {
            QDomElement att = keynode.toElement();
            QString key = att.attribute("name");

            if(cc_blacklistedKeys.contains(key))
                continue;

            if(key == "dsp.tone.eq.custom"){
                QStringList bands = att.text().split(";");
                QStringList result;
                for(auto band : bands){
                    result.append(QString::number((int)(band.toFloat() * 100)));
                }
                conf->setValue("tone_eq",QString("\"%1\"").arg(result.join(';')));
            }
            else if(key == "dsp.bs2b.mode"){
                QVector<int> coeffs = {0,0};
                switch(att.text().toInt()){
                case 0:
                    coeffs = PresetProvider::BS2B::lookupPreset("Jan Meier");
                    break;
                case 1:
                    coeffs = PresetProvider::BS2B::lookupPreset("Chu Moy");
                    break;
                case 2:
                    coeffs = PresetProvider::BS2B::lookupPreset("Default");
                    break;
                }
                conf->setValue("bs2b_fcut",coeffs.at(0));
                conf->setValue("bs2b_feed",coeffs.at(1));
            }
            else if(key == "dsp.stereowide.mode"){
                QVector<float> coeffs = {0,0};

                auto table = PresetProvider::StereoWidener::SW_LOOKUP_TABLE();
                auto key = table.keys()[att.text().toInt() + 1];
                coeffs = PresetProvider::StereoWidener::lookupPreset(key);

                conf->setValue("stereowide_mcoeff",coeffs.at(0) * 1000.f);
                conf->setValue("stereowide_scoeff",coeffs.at(1) * 1000.f);
            }
            else if(key == "dsp.analogmodelling.tubedrive"){
                conf->setValue("analogmodelling_tubedrive",att.text().toInt() * 1000.f);
            }
            else if(key == "dsp.headphone.preset"){
                auto coeffs = PresetProvider::Reverb::lookupPreset(att.text().toInt());

                conf->setValue("headset_osf",QVariant(coeffs.osf));
                conf->setValue("headset_lpf_input",QVariant(coeffs.p11));
                conf->setValue("headset_lpf_bass",QVariant(coeffs.p12));
                conf->setValue("headset_lpf_damp",QVariant(coeffs.p13));
                conf->setValue("headset_lpf_output",QVariant(coeffs.p14));
                conf->setValue("headset_reflection_amount",QVariant(coeffs.p1/100.0f));
                conf->setValue("headset_reflection_width",QVariant(coeffs.p5/100.0f));
                conf->setValue("headset_reflection_factor",QVariant(coeffs.p4/100.0f));
                conf->setValue("headset_finaldry",QVariant(coeffs.p3/10.0f));
                conf->setValue("headset_finalwet",QVariant(coeffs.p2/10.0f));
                conf->setValue("headset_width",QVariant(coeffs.p6/100.0f));
                conf->setValue("headset_wet",QVariant(coeffs.p7/10.0f));
                conf->setValue("headset_bassboost",QVariant(coeffs.p9/100.0f));
                conf->setValue("headset_lfo_spin",QVariant(coeffs.p10/100.0f));
                conf->setValue("headset_lfo_wander",QVariant(coeffs.p8/100.0f));
                conf->setValue("headset_decay",QVariant(coeffs.p15/100.0f));
                conf->setValue("headset_delay",QVariant(coeffs.p16/10.0f));
            }
            else{
                key.remove("dsp.");
                key.replace(".","_");
                conf->setValue(key,att.text());
            }
        }
    }

    conf->setValue("enable",true);
    result.configuration = ConfigIO::writeString(conf->getConfigMap());

    QString info;
    if(result.uses_ddc ||
       result.uses_irs ||
       result.uses_liveprog) info += QObject::tr("DDC, IRS and Liveprog files must be manually reselected!\n");
    result.description = info;
    return result;
}
