#include "config/DspConfig.h"
#include "Utils.h"

#include <sstream>

#include "DspHost.h"
#include "EventArgs.h"

extern "C" {
#include <JdspImpResToolbox.h>
#include <EELStdOutExtension.h>
#include <jdsp_header.h>
}

#include <QElapsedTimer>
#include <QTextStream>
#include <QDebug>
#include <cstring>
#include <assert.h>

/* C interop */
inline JamesDSPLib* cast(void* raw){
    return static_cast<JamesDSPLib*>(raw);
}
inline int32_t arySearch(int32_t *array, int32_t N, int32_t x)
{
    for (int32_t i = 0; i < N; i++)
    {
        if (array[i] == x)
            return i;
    }
    return -1;
}
#define FLOIDX 20000
inline void* GetStringForIndex(eel_string_context_state *st, float val, int32_t write)
{
    int32_t castedValue = (int32_t)(val + 0.5f);
    if (castedValue < FLOIDX)
        return 0;
    int32_t idx = arySearch(st->map, st->slot, castedValue);
    if (idx < 0)
        return 0;
    if (!write)
    {
        s_str *tmp = &st->m_literal_strings[idx];
        const char *s = s_str_c_str(tmp);
        return (void*)s;
    }
    else
        return (void*)&st->m_literal_strings[idx];
}

DspHost::DspHost(void* dspPtr, MessageHandlerFunc&& extraHandler) : _extraFunc(std::move(extraHandler))
{
    auto dsp = static_cast<JamesDSPLib*>(dspPtr);
    if(!dsp)
    {
        util::error("DspHost::ctor: Failed to initialize reference to libjamesdsp class object");
        abort();
    }

    if(!JamesDSPGetMutexStatus(dsp))
    {
        util::error("DspHost::ctor: JamesDSPGetMutexStatus returned false. "
                    "Cannot run safely in multi-threaded environment.");
        abort();
    }

    _dsp = dsp;
    _cache = new DspConfig();
}

DspHost::~DspHost()
{
    setStdOutHandler(NULL, NULL);
}

void DspHost::updateLimiter(DspConfig* config)
{
    bool releaseExists;
    bool thresholdExists;

    float limThreshold = config->get<float>(DspConfig::master_limthreshold, &releaseExists);
    float limRelease = config->get<float>(DspConfig::master_limrelease, &thresholdExists);

    if(!releaseExists || !thresholdExists)
    {
        util::warning("DspHost::updateLimiter: Limiter threshold or limiter release unset. Using defaults.");

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

    JLimiterSetCoefficients(cast(this->_dsp), limThreshold, limRelease);
}

void DspHost::updateFirEqualizer(DspConfig *config)
{
    bool typeExists;
    bool interpolationExists;

    int filterType = config->get<float>(DspConfig::tone_filtertype, &typeExists);
    int interpolationMode = config->get<float>(DspConfig::tone_interpolation, &interpolationExists);

    if(!typeExists || !interpolationExists)
    {
        util::warning("DspHost::updateFirEqualizer: Filter type or interpolation mode unset. Using defaults.");

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
        util::warning("DspHost::updateFirEqualizer: Invalid EQ data. 30 semicolon-separateds field expected, "
                      "found " + std::to_string(v.size()) + " fields instead.");
        return;
    }

    double param[30];
    for (int i = 0; i < 30; i++)
    {
        param[i] = (double)std::stod(v[i]);
    }

    FIREqualizerAxisInterpolation(cast(this->_dsp), interpolationMode, filterType, param, param + 15);
}

void DspHost::updateVdc(DspConfig *config)
{
    bool enableExists;
    bool fileExists;

    bool ddcEnable = config->get<bool>(DspConfig::ddc_enable, &enableExists);
    QString ddcFile = chopDoubleQuotes(config->get<QString>(DspConfig::ddc_file, &fileExists));

    if(!enableExists || !fileExists)
    {
        util::warning("DspHost::updateVdc: DDC file or enable switch unset. Disabling DDC engine.");

        ddcEnable = false;
    }

    if(ddcEnable)
    {
        QFile f(ddcFile);
        if(!f.exists())
        {
            util::warning("DspHost::updateVdc: Referenced file does not exist 'ddc_file'");
            return;
        }

        if (!f.open(QFile::ReadOnly | QFile::Text))
        {
            util::error("DspHost::updateVdc: Cannot open file path in property 'ddc_file'");
            util::error("DspHost::updateVdc: Disabling DDC engine");
            DDCDisable(cast(this->_dsp));
            return;
        }
        QTextStream in(&f);
        DDCStringParser(cast(this->_dsp), in.readAll().toLocal8Bit().data());

        int ret = DDCEnable(cast(this->_dsp));
        if (ret <= 0)
        {
            util::error("DspHost::updateVdc: Call to DDCEnable(this->_dsp) failed. Invalid DDC parameter?");
            util::error("DspHost::updateVdc: Disabling DDC engine");
            DDCDisable(cast(this->_dsp));
            return;
        }
    }
    else
    {
        DDCDisable(cast(this->_dsp));
    }
}

void DspHost::updateCompressor(DspConfig *config)
{
    bool maxAtkExists;
    bool maxRelExists;
    bool aggrExists;

    float maxAttack = config->get<float>(DspConfig::compression_maxatk, &maxAtkExists);
    float maxRelease = config->get<float>(DspConfig::compression_maxrel, &maxRelExists);
    float adaptSpeed = config->get<float>(DspConfig::compression_aggressiveness, &aggrExists);

    if(!maxAtkExists || !maxRelExists || !aggrExists)
    {
        util::warning("DspHost::updateLimiter: Limiter threshold or limiter release unset. Using defaults.");

        if(!maxAtkExists) maxAttack = 30;
        if(!maxRelExists) maxRelease = 200;
        if(!aggrExists) adaptSpeed = 800;
    }

    CompressorSetParam(cast(this->_dsp), maxAttack, maxRelease, adaptSpeed);
}

void DspHost::updateReverb(DspConfig* config)
{
#define GET_PARAM(key,type,defaults,msg) \
    bool key##Exists; \
    float key = config->get<type>(DspConfig::reverb_##key, &key##Exists); \
    if(!key##Exists) { \
        util::warning(msg); \
        key = defaults; \
    }

    std::string msg = "DspHost::updateReverb: At least one reverb parameter is unset. "
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

    sf_advancereverb(&cast(this->_dsp)->reverb, cast(this->_dsp)->fs, osf, reflection_amount, finalwet, finaldry,
                     reflection_factor, reflection_width, width, wet, lfo_wander, bassboost, lfo_spin,
                     lpf_input, lpf_bass, lpf_damp, lpf_output, decay, delay / 1000.0f);
#undef GET_PARAM
}

void DspHost::updateConvolver(DspConfig *config)
{
    bool fileExists;
    bool waveEditExists;
    bool optModeExists;
    bool enableExists;

    QString file = chopDoubleQuotes(config->get<QString>(DspConfig::convolver_file, &fileExists));
    QString waveEdit = chopDoubleQuotes(config->get<QString>(DspConfig::convolver_waveform_edit, &waveEditExists));
    int optMode = config->get<int>(DspConfig::convolver_optimization_mode, &optModeExists);
    bool enabled = config->get<bool>(DspConfig::convolver_enable, &enableExists);

    if(!enableExists)
    {
        util::warning("DspHost::updateConvolver: Enable switch unset. Disabling convolver.");
        enabled = false;
    }

    if(!fileExists)
    {
        util::error("DspHost::updateConvolver: convolver_file property missing. Disabling convolver.");
        enabled = false;
    }

    if(file.isEmpty())
    {
        util::error("DspHost::updateConvolver: Impulse response is empty. Disabling convolver.");
        enabled = false;
    }

    if(!optModeExists || !waveEditExists)
    {
        util::warning("DspHost::updateConvolver: Opt mode or advanced wave editing unset. Using defaults.");

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
        util::warning("DspHost::updateConvolver: Invalid advanced impulse editing data. 6 semicolon-separateds field expected, "
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

    int success = 1;

    int* impInfo = new int[2];
    float* impulse = ReadImpulseResponseToFloat(file.toLocal8Bit().constData(), cast(this->_dsp)->fs, impInfo, optMode, param);

    if(impulse == nullptr)
    {
        util::warning("DspHost::updateConvolver: Unable to read impulse response. No file selected or abnormal channel count?");

        enabled = false;

        ConvolverInfoEventArgs eventArgs;
        eventArgs.channels = -1;
        eventArgs.frames = -1;
        dispatch(ConvolverInfoChanged, eventArgs);
    }
    else
    {
        ConvolverInfoEventArgs eventArgs;
        // eventArgs.data = std::list<float>(impulse, impulse + impInfo[1] * sizeof(float));
        eventArgs.channels = impInfo[0];
        eventArgs.frames = impInfo[1];
        dispatch(ConvolverInfoChanged, eventArgs);
    }

    if(enabled)
    {
        if(impInfo[1] <= 0)
        {
            util::warning("DspHost::updateConvolver: IR is empty and has zero frames");
        }

        util::debug("DspHost::updateConvolver: Impulse response loaded: channels=" + std::to_string(impInfo[0]) + ", frames=" + std::to_string(impInfo[1]));

        Convolver1DDisable(cast(this->_dsp));
        success = Convolver1DLoadImpulseResponse(cast(this->_dsp), impulse, impInfo[0], impInfo[1]);
    }

    delete[] impInfo;
    free(impulse);

    if(enabled)
        Convolver1DEnable(cast(this->_dsp));
    else
        Convolver1DDisable(cast(this->_dsp));

    if(success <= 0)
    {
        util::debug("DspHost::updateConvolver: Failed to update convolver. Convolver1DLoadImpulseResponse returned an error.");
    }
}

void DspHost::updateGraphicEq(DspConfig *config)
{
    bool paramExists;
    bool enableExists;

    QString eq = chopDoubleQuotes(config->get<QString>(DspConfig::graphiceq_param, &paramExists));
    bool enabled = config->get<bool>(DspConfig::graphiceq_enable, &enableExists);

    if(!enableExists)
    {
        util::warning("DspHost::updateGraphicEq: Enable switch unset. Disabling graphic eq.");
        enabled = false;
    }

    if(!paramExists)
    {
        util::error("DspHost::updateGraphicEq: graphiceq_param property missing. Disabling graphic eq.");
        enabled = false;
    }

    if(enabled)
    {
        ArbitraryResponseEqualizerStringParser(cast(this->_dsp), eq.toLocal8Bit().data());
        ArbitraryResponseEqualizerEnable(cast(this->_dsp));
    }
    else
        ArbitraryResponseEqualizerDisable(cast(this->_dsp));
}

void DspHost::updateCrossfeed(DspConfig* config)
{
    bool modeExists;
    bool enableExists;
    int mode = config->get<int>(DspConfig::crossfeed_mode, &modeExists);
    int enabled = config->get<bool>(DspConfig::crossfeed_enable, &enableExists);

    if(!modeExists)
    {
        util::warning("DspHost::update: Crossfeed mode unset, using defaults");
    }

    if(!enableExists)
    {
        util::warning("DspHost::update: Crossfeed enable switch unset, disabling crossfeed.");
        enabled = false;
    }

    if(mode == 99)
    {
        bool fcutExists;
        bool feedExists;
        int fcut = config->get<int>(DspConfig::crossfeed_bs2b_fcut, &fcutExists);
        int feed = config->get<int>(DspConfig::crossfeed_bs2b_feed, &feedExists);

        if(!fcutExists)
        {
            util::warning("DspHost::update: Crossfeed custom fcut unset, using defaults");
            fcut = 650;
        }
        if(!feedExists)
        {
            util::warning("DspHost::update: Crossfeed custom feed unset, using defaults");
            feed = 95;
        }

        memset(&cast(this->_dsp)->advXF.bs2b, 0, sizeof(cast(this->_dsp)->advXF.bs2b));
        BS2BInit(&cast(this->_dsp)->advXF.bs2b[1], (unsigned int)cast(this->_dsp)->fs, ((unsigned int)fcut | ((unsigned int)feed << 16)));
        cast(this->_dsp)->advXF.mode = 1;
    }
    else
    {
       CrossfeedChangeMode(cast(this->_dsp), mode);
    }

    if(enabled)
        CrossfeedEnable(cast(this->_dsp));
    else
        CrossfeedDisable(cast(this->_dsp));
}

void DspHost::updateFromCache()
{
    update(_cache, true);
}

bool DspHost::update(DspConfig *config, bool ignoreCache)
{
    util::debug("DspHost::update called");

    QMetaEnum e = QMetaEnum::fromType<DspConfig::Key>();

    bool refreshReverb = false;
    bool refreshCrossfeed = false;
    bool refreshConvolver = false;
    bool refreshLiveprog = false;
    bool refreshGraphicEq = false;
    bool refreshVdc = false;

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
        if((isCached && cached == current) && !ignoreCache)
        {
            // Value unchanged, skip
            continue;
        }

        QString serialization;
        QDebug(&serialization) << current;
        Log::debug("DspHost::update: Property changed: " + QVariant::fromValue(key).toString() + " -> " + serialization);

        switch(key)
        {
        case DspConfig::bass_enable:
            if(current.toBool())
                BassBoostEnable(cast(this->_dsp));
            else
                BassBoostDisable(cast(this->_dsp));
            break;
        case DspConfig::bass_maxgain:
            BassBoostSetParam(cast(this->_dsp), current.toFloat());
            break;
        case DspConfig::compression_enable:
            if(current.toBool())
                CompressorEnable(cast(this->_dsp));
            else
                CompressorDisable(cast(this->_dsp));
            break;
        case DspConfig::compression_aggressiveness:
        case DspConfig::compression_maxatk:
        case DspConfig::compression_maxrel:
            updateCompressor(config);
            break;
        case DspConfig::convolver_enable:
        case DspConfig::convolver_file:
        case DspConfig::convolver_optimization_mode:
        case DspConfig::convolver_waveform_edit:
            refreshConvolver = true;
            break;
        case DspConfig::crossfeed_enable:
        case DspConfig::crossfeed_bs2b_fcut:
        case DspConfig::crossfeed_bs2b_feed:
        case DspConfig::crossfeed_mode:
            refreshCrossfeed = true;
            break;
        case DspConfig::ddc_enable:
        case DspConfig::ddc_file:
            refreshVdc = true;
            break;
        case DspConfig::graphiceq_enable:
        case DspConfig::graphiceq_param:
            refreshGraphicEq = true;
            break;
        case DspConfig::reverb_enable:
            if(current.toBool())
                ReverbEnable(cast(this->_dsp));
            else
                ReverbDisable(cast(this->_dsp));
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
        case DspConfig::liveprog_file:
            refreshLiveprog = true;
            break;
        case DspConfig::master_enable:
            dispatch(SwitchPassthrough, current.toBool());
            break;
        case DspConfig::master_limrelease:
        case DspConfig::master_limthreshold:
            updateLimiter(config);
            break;
        case DspConfig::master_postgain:
            JamesDSPSetPostGain(cast(this->_dsp), current.toFloat());
            break;
        case DspConfig::stereowide_enable:
            if(current.toBool())
                StereoEnhancementEnable(cast(this->_dsp));
            else
                StereoEnhancementDisable(cast(this->_dsp));
            break;
        case DspConfig::stereowide_level:
            StereoEnhancementSetParam(cast(this->_dsp), current.toFloat() / 100.0f);
            break;
        case DspConfig::tone_enable:
            if(current.toBool())
                FIREqualizerEnable(cast(this->_dsp));
            else
                FIREqualizerDisable(cast(this->_dsp));
            break;
        case DspConfig::tone_eq:
        case DspConfig::tone_filtertype:
        case DspConfig::tone_interpolation:
            updateFirEqualizer(config);
            break;
        case DspConfig::tube_enable:
            if(current.toBool())
                VacuumTubeEnable(cast(this->_dsp));
            else
                VacuumTubeDisable(cast(this->_dsp));
            break;
        case DspConfig::tube_pregain:
            VacuumTubeSetGain(cast(this->_dsp), current.toFloat() / 100.0f);
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

    if(refreshLiveprog)
    {
        reloadLiveprog();
    }

    if(refreshGraphicEq)
    {
        updateGraphicEq(config);
    }

    if(refreshVdc)
    {
        updateVdc(config);
    }

    if(refreshCrossfeed)
    {
        updateCrossfeed(config);
    }

    return true;
}

void DspHost::reloadLiveprog(DspConfig* config)
{
    if(config == nullptr)
    {
        config = _cache;
    }

    bool propExists;
    bool enableExists;
    QString file = chopDoubleQuotes(config->get<QString>(DspConfig::liveprog_file, &propExists));
    bool enabled = config->get<bool>(DspConfig::liveprog_enable, &enableExists);

    if(!enableExists)
    {
        util::warning("DspHost::refreshLiveprog: Liveprog enable switch unset. Disabling liveprog.");
        enabled = false;
    }

    if(!propExists)
    {
        util::warning("DspHost::refreshLiveprog: liveprog_file property not found in cache. Disabling liveprog.");
        enabled = false;
    }

    // Attach log listener
    setStdOutHandler(receiveLiveprogStdOut, this);

    QFile f(file);
    if(!f.exists())
    {
        util::warning("DspHost::refreshLiveprog: Referenced file does not exist anymore. Disabling liveprog.");
        enabled = false;
    }

    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        util::error("DspHost::refreshLiveprog: Cannot open file path. Disabling liveprog.");
        enabled = false;
    }
    QTextStream in(&f);


    LiveProgDisable(cast(this->_dsp));
    dispatch(EelCompilerStart, f.fileName());

    QElapsedTimer timer;
    timer.start();
    int ret = LiveProgStringParser(cast(this->_dsp), in.readAll().toLocal8Bit().data());

    // Workaround due to library bug
    jdsp_unlock(cast(this->_dsp));

    float msecs = timer.nsecsElapsed() / 1000000.0;

    const char* errorString = NSEEL_code_getcodeerror(cast(this->_dsp)->eel.vm);
    if(errorString != NULL)
    {
        util::warning("DspHost::refreshLiveprog: NSEEL_code_getcodeerror: Syntax error in script file, cannot load. Reason: " + std::string(errorString));
    }
    if(ret <= 0)
    {
        util::warning("DspHost::refreshLiveprog: " + std::string(checkErrorCode(ret)));
    }

    QList<QString> resultArgs;
    resultArgs.append(QString::number(ret));
    resultArgs.append(errorString == NULL ? "" : errorString);
    resultArgs.append(f.fileName());
    resultArgs.append(QString::number(msecs));
    resultArgs.append(checkErrorCode(ret));
    dispatch(EelCompilerResult, resultArgs);

    if(enabled)
        LiveProgEnable(cast(this->_dsp));
    else
        LiveProgDisable(cast(this->_dsp));
}

std::list<EelVariable> DspHost::enumEelVariables()
{
    std::list<EelVariable> vars;

    compileContext *c = (compileContext*)cast(this->_dsp)->eel.vm;
    for (int i = 0; i < c->varTable_numBlocks; i++)
    {
        for (int j = 0; j < NSEEL_VARS_PER_BLOCK; j++)
        {
            EelVariable var;
            char *valid = (char*)GetStringForIndex(c->m_string_context, c->varTable_Values[i][j], 0);
            var.isString = valid;

            if (c->varTable_Names[i][j])
            {
                var.name = c->varTable_Names[i][j];
                if(var.isString)
                    var.value = valid;
                else
                    var.value = c->varTable_Values[i][j];

                vars.push_back(var);
            }
        }
    }

    return vars;
}

void DspHost::dispatch(Message msg, std::any value)
{
    _extraFunc(msg, value);
}

void receiveLiveprogStdOut(const char *buffer, void* userData)
{
    DspHost* self = static_cast<DspHost*>(userData);
    assert(self != nullptr);

    self->dispatch(DspHost::EelWriteOutputBuffer, QString(buffer));
}

