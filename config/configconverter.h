#ifndef CONFIGCONVERTER_H
#define CONFIGCONVERTER_H

#include <QObject>

typedef struct{
    QString configuration = "";
    QString description = "";
    bool failed = false;
    bool uses_irs = false;
    bool uses_ddc = false;
    bool uses_liveprog = false;
} ConversionResult;

static const QStringList cc_blacklistedKeys = QStringList({"dsp.masterswitch.enable","dsp.tone.eq",
                                                           "dsp.convolver.resampler","dsp.strph.enable",
                                                           "dsp.streq.enable","dsp.convolver.quality",
                                                           "dsp.convolver.files","dsp.strph.stringleft",
                                                           "dsp.streq.stringp","dsp.streq.filtertype",
                                                           "dsp.strph.stringright","dsp.exact71.enable",
                                                           "dsp.ddc.files","dsp.liveprog.files",
                                                           "dsp.tone.eq"});

class ConfigConverter
{
public:
    static ConversionResult fromAndroid(QString path);
};



#endif // CONFIGCONVERTER_H
