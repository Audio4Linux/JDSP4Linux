#include "config/DspConfig.h"

#include "JamesDspElement.h"

#include <jdsp_extensions.h>

JamesDspElement::JamesDspElement() : FilterElement("jamesdsp", "jamesdsp")
{
    gboolean gEnabled;
    gpointer gDspPtr = NULL;
    this->getValues("dsp_ptr", &gDspPtr,
                    "dsp_enable", &gEnabled, NULL);

    assert(!gEnabled); // check if underlying object is fresh

    _dsp = static_cast<JamesDSPLib*>(gDspPtr);
    if(!_dsp)
    {
        util::error("JamesDspElement::ctor: Failed to initialize reference to libjamesdsp class object");
        abort();
    }

    if(!JamesDSPGetMutexStatus(_dsp))
    {
        util::error("JamesDspElement::ctor: JamesDSPGetMutexStatus returned false. "
                    "Cannot run safely in multi-threaded environment.");
        abort();
    }

    _cache = new DspConfig();

    auto temp = new DspConfig();
    temp->set(DspConfig::master_enable, true);
    temp->set(DspConfig::bass_enable, false);
    temp->set(DspConfig::bass_maxgain, 10);
    temp->set(DspConfig::crossfeed_enable, false);
    temp->set(DspConfig::crossfeed_mode, 3);

    temp->set(DspConfig::Key::liveprog_enable, true);
    temp->set(DspConfig::Key::liveprog_file, "/home/tim/.config/jamesdsp/liveprog/phaseshifter.eel");
    update(temp);
    delete temp;
}

void JamesDspElement::updateLimiter(DspConfig* config)
{
    bool releaseExists;
    bool thresholdExists;

    float limThreshold = config->get<float>(DspConfig::master_limrelease, &releaseExists);
    float limRelease = config->get<float>(DspConfig::master_limthreshold, &thresholdExists);

    if(!releaseExists || !thresholdExists)
    {
        util::warning("JamesDspElement::updateLimiter: Limiter threshold or limiter release unset. Using defaults.");

        if(!releaseExists) limRelease = -0.1;
        else if (!thresholdExists) limThreshold = 60;
    }

    if (limThreshold > -0.09)
    {
        limThreshold = -0.09;
    }
    if (limRelease < 0.15)
    {
        limRelease = 0.15;
    }

    JLimiterSetCoefficients(this->_dsp, limThreshold, limRelease);
}

void JamesDspElement::updateFirEqualizer(DspConfig *config)
{
    bool typeExists;
    bool interpolationExists;

    int filterType = config->get<float>(DspConfig::tone_filtertype, &typeExists);
    int interpolationMode = config->get<float>(DspConfig::tone_interpolation, &interpolationExists);

    if(!typeExists || !interpolationExists)
    {
        util::warning("JamesDspElement::updateFirEqualizer: Filter type or interpolation mode unset. Using defaults.");

        if(!typeExists)  filterType = 0;
        else if (!interpolationExists) interpolationMode = 0;
    }

    std::string str = config->get<std::string>(DspConfig::tone_eq);
    std::vector<string> v;
    std::stringstream ss(str);

    while (ss.good()) {
        std::string substr;
        getline(ss, substr, ';');
        v.push_back(substr);
    }

    if(v.size() != 30)
    {
        util::warning("JamesDspElement::updateFirEqualizer: Invalid EQ data. 30 semicolon-separateds field expected, "
                      "found " + std::to_string(v.size()) + " fields instead.");
        return;
    }

    double param[30];
    for (int i = 0; i < 30; i++)
    {
        param[i] = (double)std::stod(v[i]);
    }

    FIREqualizerAxisInterpolation(this->_dsp, interpolationMode, filterType, param, param + 15);
}

void JamesDspElement::updateVdc(DspConfig *config)
{
    bool enableExists;
    bool fileExists;

    bool ddcEnable = config->get<bool>(DspConfig::ddc_enable, &enableExists);
    QString ddcFile = config->get<QString>(DspConfig::ddc_file, &fileExists);

    if(!enableExists || !fileExists)
    {
        util::warning("JamesDspElement::updateVdc: DDC file or enable switch unset. Disabling DDC engine.");

        ddcEnable = false;
    }

    if(ddcEnable)
    {
        QFile f(ddcFile);
        if (!f.open(QFile::ReadOnly | QFile::Text))
        {
            util::error("JamesDspElement::updateVdc: Cannot open file path in property 'ddc_file'");
            util::error("JamesDspElement::updateVdc: Disabling DDC engine");
            DDCDisable(this->_dsp);
            return;
        }
        QTextStream in(&f);
        DDCStringParser(this->_dsp, in.readAll().toLocal8Bit().data());

        int ret = DDCEnable(this->_dsp);
        if (ret <= 0)
        {
            util::error("JamesDspElement::updateVdc: Call to DDCEnable(this->_dsp) failed. Invalid DDC parameter?");
            util::error("JamesDspElement::updateVdc: Disabling DDC engine");
            DDCDisable(this->_dsp);
            return;
        }
    }
    else
    {
        DDCDisable(this->_dsp);
    }
}

void JamesDspElement::updateCompressor(DspConfig *config)
{
    bool maxAtkExists;
    bool maxRelExists;
    bool aggrExists;

    float maxAttack = config->get<float>(DspConfig::compression_maxatk, &maxAtkExists);
    float maxRelease = config->get<float>(DspConfig::compression_maxrel, &maxRelExists);
    float adaptSpeed = config->get<float>(DspConfig::compression_aggressiveness, &aggrExists);

    if(!maxAtkExists || !maxRelExists || !aggrExists)
    {
        util::warning("JamesDspElement::updateLimiter: Limiter threshold or limiter release unset. Using defaults.");

        if(!maxAtkExists) maxAttack = 30;
        if(!maxRelExists) maxRelease = 200;
        if(!aggrExists) adaptSpeed = 800;
    }

    CompressorSetParam(this->_dsp, maxAttack, maxRelease, adaptSpeed);
}

void JamesDspElement::updateReverb(DspConfig* config)
{
    sf_advancereverb(&this->_dsp->reverb, this->_dsp->fs, 1, 1, 1, 1, 1,1,1,1,1,1,1,1,1,1,1,1,1);

}

bool JamesDspElement::update(DspConfig *config)
{
    QMetaEnum e = QMetaEnum::fromType<DspConfig::Key>();

    bool refreshReverb = false;
    for (int k = 0; k < e.keyCount(); k++)
    {
        DspConfig::Key key = (DspConfig::Key) e.value(k);
        DspConfig::Type type = config->type(key);

        if(type == DspConfig::Type::Unknown)
        {
            // Value uninitialized, skip
            continue;
        }

        bool isCached = false;
        QVariant cached = _cache->get<QVariant>(key, &isCached);
        QVariant current = config->get<QVariant>(key);
        if(isCached && cached == current)
        {
            // Value unchanged, skip
            continue;
        }

        qDebug() << QVariant::fromValue(key).toString() << current;

        switch(key)
        {
        case DspConfig::bass_enable:
            if(current.toBool())
                BassBoostEnable(this->_dsp);
            else
                BassBoostDisable(this->_dsp);
            break;
        case DspConfig::bass_maxgain:
            BassBoostSetParam(this->_dsp, current.toFloat());
            break;
        case DspConfig::compression_enable:
            if(current.toBool())
                CompressorEnable(this->_dsp);
            else
                CompressorDisable(this->_dsp);
            break;
        case DspConfig::compression_aggressiveness:
        case DspConfig::compression_maxatk:
        case DspConfig::compression_maxrel:
            updateCompressor(config);
            break;
        case DspConfig::convolver_enable:
            if(current.toBool())
                Convolver1DEnable(this->_dsp);
            else
                Convolver1DDisable(this->_dsp);
            break;
        case DspConfig::convolver_file:
            break;
        case DspConfig::convolver_optimization_mode:
            break;
        case DspConfig::convolver_waveform_edit:
            break;
        case DspConfig::crossfeed_bs2b_fcut:
            break;
        case DspConfig::crossfeed_bs2b_feed:
            break;
        case DspConfig::crossfeed_enable:
            if(current.toBool())
                CrossfeedEnable(this->_dsp);
            else
                CrossfeedDisable(this->_dsp);
            break;
        case DspConfig::crossfeed_mode:
            CrossfeedChangeMode(this->_dsp, current.toInt());
            break;
        case DspConfig::ddc_enable:
        case DspConfig::ddc_file:
            updateVdc(config);
            break;
        case DspConfig::graphiceq_enable:
            if(current.toBool())
                ArbitraryResponseEqualizerEnable(this->_dsp);
            else
                ArbitraryResponseEqualizerDisable(this->_dsp);
            break;
        case DspConfig::graphiceq_param:
            ArbitraryResponseEqualizerStringParser(this->_dsp, current.toString().toLocal8Bit().data());
            break;
        case DspConfig::reverb_enable:
            if(current.toBool())
                ReverbEnable(this->_dsp);
            else
                ReverbDisable(this->_dsp);
            break;
        case DspConfig::reverb_bassboost:
        case DspConfig::reverb_decay:
        case DspConfig::reverb_delay:
        case DspConfig::reverb_finaldry:
        case DspConfig::reverb_finalwet:
        case DspConfig::reverb_lfo_spin:
        case DspConfig::reverb_lfo_wander:
        case DspConfig::reverb_lpf_bass:
        case DspConfig::reverb_lpf_damp:
        case DspConfig::reverb_lpf_input:
        case DspConfig::reverb_lpf_output:
        case DspConfig::reverb_osf:
        case DspConfig::reverb_reflection_amount:
        case DspConfig::reverb_reflection_factor:
        case DspConfig::reverb_reflection_width:
        case DspConfig::reverb_wet:
        case DspConfig::reverb_width:
            refreshReverb = true;
            break;
        case DspConfig::liveprog_enable:
            if(current.toBool())
                LiveProgEnable(this->_dsp);
            else
                LiveProgDisable(this->_dsp);
            break;
        case DspConfig::liveprog_file: {
                QFile f(current.toString());
                if (!f.open(QFile::ReadOnly | QFile::Text))
                {
                    util::error("JamesDspElement::update: Cannot open file path in property 'liveprog_file'");
                    break;
                }
                QTextStream in(&f);

                int ret = LiveProgStringParser(this->_dsp, in.readAll().toLocal8Bit().data());
                if(ret <= 0)
                {
                    util::error("LiveProgStringParser: Syntax error in script file, cannot load. Reason: " + std::string(checkErrorCode(ret)));
                }
            }
            break;
        case DspConfig::master_enable:
            this->setValues("dsp_enable", current.toBool(), NULL);
            break;
        case DspConfig::master_limrelease:
        case DspConfig::master_limthreshold:
            updateLimiter(config);
            break;
        case DspConfig::master_postgain:
            JamesDSPSetPostGain(this->_dsp, current.toFloat());
            break;
        case DspConfig::stereowide_enable:
            if(current.toBool())
                StereoEnhancementEnable(this->_dsp);
            else
                StereoEnhancementDisable(this->_dsp);
            break;
        case DspConfig::stereowide_level:
            StereoEnhancementSetParam(this->_dsp, current.toFloat() / 100.0f);
            break;
        case DspConfig::tone_enable:
            if(current.toBool())
                FIREqualizerEnable(this->_dsp);
            else
                FIREqualizerDisable(this->_dsp);
            break;
        case DspConfig::tone_eq:
        case DspConfig::tone_filtertype:
        case DspConfig::tone_interpolation:
            updateFirEqualizer(config);
            break;
        case DspConfig::tube_enable:
            if(current.toBool())
                VacuumTubeEnable(this->_dsp);
            else
                VacuumTubeDisable(this->_dsp);
            break;
        case DspConfig::tube_pregain:
            VacuumTubeSetGain(this->_dsp, current.toFloat() / 1000.0f);
            break;
        }

        _cache->set(key, current);
    }

    if(refreshReverb)
    {
        updateReverb(config);
    }

    return true;
}
