#include "config/DspConfig.h"

#include "JamesDspElement.h"

extern "C" {
#include <JdspImpResToolbox.h>
}

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

    std::string str = chopDoubleQuotes(config->get<QString>(DspConfig::tone_eq)).toStdString();
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
    QString ddcFile = chopDoubleQuotes(config->get<QString>(DspConfig::ddc_file, &fileExists));

    if(!enableExists || !fileExists)
    {
        util::warning("JamesDspElement::updateVdc: DDC file or enable switch unset. Disabling DDC engine.");

        ddcEnable = false;
    }

    if(ddcEnable)
    {
        QFile f(ddcFile);
        if(!f.exists())
        {
            util::warning("JamesDspElement::updateVdc: Referenced file does not exist 'ddc_file'");
            return;
        }

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
#define GET_PARAM(key,type,defaults,msg) \
    bool key##Exists; \
    float key = config->get<type>(DspConfig::reverb_##key, &key##Exists); \
    if(!key##Exists) { \
        util::warning(msg); \
        key = defaults; \
    }

    std::string msg = "JamesDspElement::updateReverb: At least one reverb parameter is unset. "
                      "Attempting to fill out with defaults; this may cause unexpected audio changes.";

    GET_PARAM(bassboost, float, 0.15, msg);
    GET_PARAM(decay, float, 3.2, msg);
    GET_PARAM(delay, float, 20, msg);
    GET_PARAM(finaldry, float, -7.0, msg);
    GET_PARAM(finalwet, float, -9.0, msg);
    GET_PARAM(lfo_spin, float, 0.7, msg);
    GET_PARAM(lfo_wander, float, 0.25, msg);
    GET_PARAM(lpf_bass, int, 500, msg);
    GET_PARAM(lpf_damp, int, 7000, msg);
    GET_PARAM(lpf_input, int, 17000, msg);
    GET_PARAM(lpf_output, int, 10000, msg);
    GET_PARAM(osf, int, 1, msg);
    GET_PARAM(reflection_amount, float, 0.40, msg);
    GET_PARAM(reflection_factor, float, 1.6, msg);
    GET_PARAM(reflection_width, float, 0.7, msg);
    GET_PARAM(wet, float, 0, msg);
    GET_PARAM(width, float, 1.0, msg);

    sf_advancereverb(&this->_dsp->reverb, this->_dsp->fs, osf, reflection_amount, finalwet, finaldry,
                     reflection_factor, reflection_width, width, wet, lfo_wander, bassboost, lfo_spin,
                     lpf_input, lpf_bass, lpf_damp, lpf_output, decay, delay / 1000.0f);
#undef GET_PARAM
}

void JamesDspElement::updateConvolver(DspConfig *config)
{
    bool fileExists;
    bool waveEditExists;
    bool optModeExists;

    QString file = chopDoubleQuotes(config->get<QString>(DspConfig::convolver_file, &fileExists));
    QString waveEdit = chopDoubleQuotes(config->get<QString>(DspConfig::convolver_waveform_edit, &waveEditExists));
    int optMode = config->get<int>(DspConfig::convolver_optimization_mode, &optModeExists);

    if(!fileExists)
    {
        util::error("JamesDspElement::updateConvolver: convolver_file property missing. Cannot update convolver state.");
        return;
    }

    if(file.isEmpty())
    {
        return;
    }

    if(!optModeExists || !waveEditExists)
    {
        util::warning("JamesDspElement::updateConvolver: Opt mode or advanced wave editing unset. Using defaults.");

        if(!optModeExists) optMode = 0;
        if(!waveEditExists) waveEdit = "-80;-100;23;12;17;28";
    }

    std::vector<string> v;
    std::stringstream ss(waveEdit.toStdString());

    while (ss.good()) {
        std::string substr;
        getline(ss, substr, ';');
        v.push_back(substr);
    }

    int param[6];
    if(v.size() != 6)
    {
        util::warning("JamesDspElement::updateConvolver: Invalid advanced impulse editing data. 6 semicolon-separateds field expected, "
                      "found " + std::to_string(v.size()) + " fields instead.");

        param[0] = -80;
        param[1] = -100;
        param[2] = 23;
        param[3] = 12;
        param[4] = 17;
        param[5] = 28;
    }
    else
    {
        for (int i = 0; i < 6; i++)
        {
            param[i] = (int)std::stoi(v[i]);
        }
    }

    int* impInfo = new int[2];
    float* impulse = ReadImpulseResponseToFloat(file.toLocal8Bit().constData(), this->_dsp->fs, impInfo, optMode, param);

    if(impInfo[1] <= 0)
    {
        util::warning("JamesDspElement::updateConvolver: IR is empty and has zero frames");
    }

    util::debug("JamesDspElement::updateConvolver: Impulse response loaded: channels=" + std::to_string(impInfo[0]) + ", frames=" + std::to_string(impInfo[1]));

    int success = Convolver1DLoadImpulseResponse(this->_dsp, impulse, impInfo[0], impInfo[1]);

    if(success <= 0)
    {
        util::debug("JamesDspElement::updateConvolver: Failed to update convolver. Convolver1DLoadImpulseResponse returned an error.");
    }

    delete[] impInfo;
    free(impulse);
}

bool JamesDspElement::update(DspConfig *config)
{
    util::debug("JamesDspElement::update called");

    QMetaEnum e = QMetaEnum::fromType<DspConfig::Key>();

    bool refreshReverb = false;
    bool refreshCrossfeed = false;
    bool refreshConvolver = false;
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
        case DspConfig::convolver_optimization_mode:
        case DspConfig::convolver_waveform_edit:
            refreshConvolver = true;
            break;
        case DspConfig::crossfeed_enable:
            if(current.toBool())
                CrossfeedEnable(this->_dsp);
            else
                CrossfeedDisable(this->_dsp);
            break;
        case DspConfig::crossfeed_bs2b_fcut:
        case DspConfig::crossfeed_bs2b_feed:
        case DspConfig::crossfeed_mode:
            refreshCrossfeed = true;
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
            ArbitraryResponseEqualizerStringParser(this->_dsp, chopDoubleQuotes(current.toString()).toLocal8Bit().data());
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
                QFile f(chopDoubleQuotes(current.toString()));
                if(!f.exists())
                {
                    util::warning("JamesDspElement::update: Referenced file does not exist 'liveprog_file'");
                    break;
                }

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

    if(refreshConvolver)
    {
        updateConvolver(config);
    }

    if(refreshCrossfeed)
    {
        bool modeExists;
        int mode = config->get<int>(DspConfig::crossfeed_mode, &modeExists);

        if(!modeExists)
        {
            util::warning("JamesDspElement::update: Crossfeed mode unset, using defaults");
        }

        if(mode == 99)
        {
            bool fcutExists;
            bool feedExists;
            int fcut = config->get<int>(DspConfig::crossfeed_bs2b_fcut, &fcutExists);
            int feed = config->get<int>(DspConfig::crossfeed_bs2b_feed, &feedExists);

            if(!fcutExists)
            {
                util::warning("JamesDspElement::update: Crossfeed custom fcut unset, using defaults");
                fcut = 650;
            }
            if(!feedExists)
            {
                util::warning("JamesDspElement::update: Crossfeed custom feed unset, using defaults");
                feed = 95;
            }

            memset(&this->_dsp->advXF.bs2b, 0, sizeof(this->_dsp->advXF.bs2b));
            BS2BInit(&this->_dsp->advXF.bs2b[1], (unsigned int)this->_dsp->fs, ((unsigned int)fcut | ((unsigned int)feed << 16)));
            this->_dsp->advXF.mode = 1;
        }
        else
        {
           CrossfeedChangeMode(this->_dsp, mode);
        }
    }

    return true;
}

void JamesDspElement::reloadLiveprog()
{
    bool propExists;
    QString file = chopDoubleQuotes(_cache->get<QString>(DspConfig::liveprog_file, &propExists));

    if(!propExists)
    {
        util::warning("JamesDspElement::refreshLiveprog: liveprog_file property not found in cache. Cannot reload.");
        return;
    }

    QFile f(file);
    if(!f.exists())
    {
        util::warning("JamesDspElement::refreshLiveprog: Referenced file does not exist anymore. Cannot reload.");
        return;
    }

    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        util::error("JamesDspElement::refreshLiveprog: Cannot open file path");
        return;
    }
    QTextStream in(&f);

    int ret = LiveProgStringParser(this->_dsp, in.readAll().toLocal8Bit().data());
    if(ret <= 0)
    {
        util::error("JamesDspElement::refreshLiveprog: Syntax error in script file, cannot load. Reason: " + std::string(checkErrorCode(ret)));
    }
    // TODO report liveprog result
}
