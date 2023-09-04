#include "config/DspConfig.h"
#include "Utils.h"
#include "BenchmarkWorker.h"

#include <QTimer>
#include <sstream>
#include <string>

#include "DspHost.h"
#include "EventArgs.h"

extern "C" {
#ifdef DEBUG_FPE
#include <fenv.h>
#endif
#include <JdspImpResToolbox.h>
#include <EELStdOutExtension.h>
#include <jdsp_header.h>
}

#include <QElapsedTimer>
#include <QTextStream>
#include <QThread>
#include <QDebug>
#include <cstring>
#include <assert.h>

/* C interop */
inline JamesDSPLib* cast(void* raw){
    return static_cast<JamesDSPLib*>(raw);
}

DspHost::DspHost(void* dspPtr, MessageHandlerFunc&& extraHandler) : _extraFunc(std::move(extraHandler))
{
    auto dsp = static_cast<JamesDSPLib*>(dspPtr);
    if(!dsp)
    {
        util::error("Failed to initialize reference to libjamesdsp class object");
        abort();
    }

    if(!JamesDSPGetMutexStatus(dsp))
    {
        util::error("JamesDSPGetMutexStatus returned false. "
                    "Cannot run safely in multi-threaded environment.");
        abort();
    }

    _dsp = dsp;
    _cache = new DspConfig();

    benchmarkThread = new QThread();
    benchmarkWorker = new BenchmarkWorker();

    QObject::connect(benchmarkThread, &QThread::started, benchmarkWorker, &BenchmarkWorker::process);
    QObject::connect(benchmarkWorker, &BenchmarkWorker::finished, benchmarkThread, &QThread::quit);
    QObject::connect(benchmarkWorker, &BenchmarkWorker::finished, benchmarkThread, [this] {
        dispatch(Message::BenchmarkDone, nullptr);
    });

    loadBenchmarkData();

    if(AppConfig::instance().get<bool>(AppConfig::BenchmarkOnBoot))
        QTimer::singleShot(30000, [this]{ runBenchmarks(); });
}

DspHost::~DspHost()
{
    setStdOutHandler(NULL, NULL);

    _cache->deleteLater();
    benchmarkThread->deleteLater();
    benchmarkWorker->deleteLater();

    _cache = nullptr;
    benchmarkWorker = nullptr;
    benchmarkThread = nullptr;
}

void DspHost::runBenchmarks()
{
    if(benchmarkThread->isRunning()) {
        Log::warning("Another benchmark is already active");
        return;
    }

    benchmarkWorker->moveToThread(benchmarkThread);
    benchmarkThread->start();
}

void DspHost::loadBenchmarkData()
{
    // Empty cache = no benchmark data stored
    if(AppConfig::instance().get<QString>(AppConfig::BenchmarkCacheC0).isEmpty() &&
       AppConfig::instance().get<QString>(AppConfig::BenchmarkCacheC1).isEmpty())
        return;

    auto loadFromCache = [](int num, double* output){
        auto values = AppConfig::instance().get<QString>(num == 0 ? AppConfig::BenchmarkCacheC0 : AppConfig::BenchmarkCacheC1).split(';', Qt::SkipEmptyParts);
        if(values.size() != MAX_BENCHMARK)
            return false;

        for(int i = 0; i < values.size(); i++) {
            bool ok = false;
            output[i] = values[i].toDouble(&ok);
            if(!ok)
                return false;
        }
        return true;
    };

    double* c0 = new double[MAX_BENCHMARK];
    double* c1 = new double[MAX_BENCHMARK];

    bool ok0 = loadFromCache(0, c0);
    bool ok1 = loadFromCache(1, c1);

    if(!ok0 || !ok1) {
        Log::error("Failed to parse cached benchmark data");
        return;
    }

    JamesDSP_Load_benchmark(c0, c1);

    delete[] c0;
    delete[] c1;
}

void DspHost::updateLimiter(DspConfig* config)
{
    bool releaseExists;
    bool thresholdExists;

    float limThreshold = config->get<float>(DspConfig::master_limthreshold, &releaseExists);
    float limRelease = config->get<float>(DspConfig::master_limrelease, &thresholdExists);

    if(!releaseExists || !thresholdExists)
    {
        util::warning("Limiter threshold or limiter release unset. Using defaults.");

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
        util::warning("Filter type or interpolation mode unset. Using defaults.");

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
        util::warning("Invalid EQ data. 30 semicolon-separateds field expected, "
                      "found " + std::to_string(v.size()) + " fields instead.");
        return;
    }

    double param[30];
    for (int i = 0; i < 30; i++)
    {
        param[i] = (double)std::stod(v[i]);
    }

    MultimodalEqualizerAxisInterpolation(cast(this->_dsp), interpolationMode, filterType, param, param + 15);
}

void DspHost::updateVdc(DspConfig *config)
{
    bool enableExists;
    bool fileExists;

    bool ddcEnable = config->get<bool>(DspConfig::ddc_enable, &enableExists);
    QString ddcFile = chopDoubleQuotes(config->get<QString>(DspConfig::ddc_file, &fileExists));

    if(!enableExists || !fileExists)
    {
        util::warning("DDC file or enable switch unset. Disabling DDC engine.");

        ddcEnable = false;
    }

    if(ddcEnable)
    {
        QFile f(ddcFile);
        if(!f.exists())
        {
            util::warning("Referenced file does not exist 'ddc_file'");
            return;
        }

        if (!f.open(QFile::ReadOnly | QFile::Text))
        {
            util::error("Cannot open file path in property 'ddc_file'");
            util::error("Disabling DDC engine");
            DDCDisable(cast(this->_dsp));
            return;
        }
        QTextStream in(&f);
        DDCStringParser(cast(this->_dsp), in.readAll().toLocal8Bit().data());

        int ret = DDCEnable(cast(this->_dsp), 1);
        if (ret <= 0)
        {
            util::error("Call to DDCEnable(this->_dsp) failed. Invalid DDC parameter?");
            util::error("Disabling DDC engine");
            DDCDisable(cast(this->_dsp));
            return;
        }
    }
    else
    {
        DDCDisable(cast(this->_dsp));
    }
}

void DspHost::updateCompander(DspConfig *config)
{
    int granularity = config->get<int>(DspConfig::compander_granularity);
    float timeconstant = config->get<float>(DspConfig::compander_timeconstant);
    int tftransforms = config->get<int>(DspConfig::compander_time_freq_transforms);

    std::string str = chopDoubleQuotes(config->get<QString>(DspConfig::compander_response)).toStdString();
    std::vector<string> v;
    std::stringstream ss(str);

    while (ss.good()) {
        std::string substr;
        getline(ss, substr, ';');
        v.push_back(substr);
    }

    if(v.size() != 14)
    {
        util::warning("Invalid compander data. 14 semicolon-separateds field expected, "
                      "found " + std::to_string(v.size()) + " fields instead.");
        return;
    }

    double param[14];
    for (int i = 0; i < 14; i++)
    {
        param[i] = (double)std::stod(v[i]);
    }

    CompressorSetParam(cast(this->_dsp), timeconstant, granularity, tftransforms, 0);
    CompressorSetGain(cast(this->_dsp), param, param + 7, 1);
}

void DspHost::updateStereoWide(DspConfig *config)
{
    if(config->get<bool>(DspConfig::stereowide_enable))
        StereoEnhancementEnable(cast(this->_dsp));
    else
        StereoEnhancementDisable(cast(this->_dsp));

    StereoEnhancementSetParam(cast(this->_dsp), config->get<float>(DspConfig::stereowide_level) / 100.0f);
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

    std::string msg = "At least one reverb parameter is unset. "
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
        util::warning("Enable switch unset. Disabling convolver.");
        enabled = false;
    }

    if(!fileExists)
    {
        util::error("convolver_file property missing. Disabling convolver.");
        enabled = false;
    }

    if(file.isEmpty())
    {
        util::error("Impulse response is empty. Disabling convolver.");
        enabled = false;
    }

    if(!optModeExists || !waveEditExists)
    {
        util::warning("Opt mode or advanced wave editing unset. Using defaults.");

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
        util::warning("Invalid advanced impulse editing data. 6 semicolon-separateds field expected, "
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
        util::warning("Unable to read impulse response. No file selected or abnormal channel count?");

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
            util::warning("IR is empty and has zero frames");
        }

        util::debug("Impulse response loaded: channels=" + std::to_string(impInfo[0]) + ", frames=" + std::to_string(impInfo[1]));

        Convolver1DDisable(cast(this->_dsp));
        success = Convolver1DLoadImpulseResponse(cast(this->_dsp), impulse, impInfo[0], impInfo[1], 1);
    }

    delete[] impInfo;
    free(impulse);

    if(enabled)
        Convolver1DEnable(cast(this->_dsp));
    else
        Convolver1DDisable(cast(this->_dsp));

    if(success <= 0)
    {
        util::debug("Failed to update convolver. Convolver1DLoadImpulseResponse returned an error.");
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
        util::warning("Enable switch unset. Disabling graphic eq.");
        enabled = false;
    }

    if(!paramExists)
    {
        util::error("graphiceq_param property missing. Disabling graphic eq.");
        enabled = false;
    }

    if(enabled)
    {
        ArbitraryResponseEqualizerStringParser(cast(this->_dsp), eq.toLocal8Bit().data());
        ArbitraryResponseEqualizerEnable(cast(this->_dsp), 1);
    }
    else
        ArbitraryResponseEqualizerDisable(cast(this->_dsp));
}

void DspHost::updateCrossfeed(DspConfig* config)
{
    int mode = config->get<int>(DspConfig::crossfeed_mode);

    // Workaround: CrossfeedChangeMode for mode 0 (weak) leads to audio loss. so let's just do it directly like BS2B custom above
    if(mode == 99 /* custom */ || mode == 0 /* weak */)
    {
        int fcut = config->get<int>(DspConfig::crossfeed_bs2b_fcut);
        int feed = config->get<int>(DspConfig::crossfeed_bs2b_feed);

        // Workaround, see comment above
        if(mode == 0)
        {
            fcut = 700;
            feed = 65;
        }

        memset(&cast(this->_dsp)->advXF.bs2b, 0, sizeof(cast(this->_dsp)->advXF.bs2b));
        BS2BInit(&cast(this->_dsp)->advXF.bs2b[1], (unsigned int)cast(this->_dsp)->fs, ((unsigned int)fcut | ((unsigned int)feed << 16)));
        cast(this->_dsp)->advXF.mode = 1;
    }
    else
    {        
       CrossfeedChangeMode(cast(this->_dsp), mode);
    }
}

void DspHost::updateFromCache()
{
    update(_cache, true);
}

bool DspHost::update(DspConfig *config, bool ignoreCache)
{
    util::debug("Config update started");

    QMetaEnum e = QMetaEnum::fromType<DspConfig::Key>();

    bool refreshReverb = false;
    bool refreshCrossfeed = false;
    bool refreshConvolver = false;
    bool refreshLiveprog = false;
    bool refreshGraphicEq = false;
    bool refreshVdc = false;
    bool refreshCompander = false;
    bool refreshStereoWide = false;

#ifdef DEBUG_FPE
    feenableexcept(FE_ALL_EXCEPT & ~FE_INEXACT & ~FE_INVALID);
#endif

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
        QVariant cached = _cache->get<QVariant>(key, &isCached, false);
        QVariant current = config->get<QVariant>(key);
        if((isCached && cached == current) && !ignoreCache)
        {
            // Value unchanged, skip
            continue;
        }

        QString serialization;
        QDebug(&serialization) << current;
        Log::debug("Property changed: " + QVariant::fromValue(key).toString() + " -> " + serialization);

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
        case DspConfig::compander_enable:
            if(current.toBool())
                CompressorEnable(cast(this->_dsp), 1);
            else
                CompressorDisable(cast(this->_dsp));
            break;
        case DspConfig::compander_granularity:
        case DspConfig::compander_response:
        case DspConfig::compander_time_freq_transforms:
        case DspConfig::compander_timeconstant:
            refreshCompander = true;
            break;
        case DspConfig::convolver_enable:
        case DspConfig::convolver_file:
        case DspConfig::convolver_optimization_mode:
        case DspConfig::convolver_waveform_edit:
            refreshConvolver = true;
            break;
        case DspConfig::crossfeed_enable:
            if(current.toBool())
                CrossfeedEnable(cast(this->_dsp), 1);
            else
                CrossfeedDisable(cast(this->_dsp));
            break;
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
        case DspConfig::stereowide_level:
            refreshStereoWide = true;
            break;
        case DspConfig::tone_enable:
            if(current.toBool())
                MultimodalEqualizerEnable(cast(this->_dsp), 1);
            else
                MultimodalEqualizerDisable(cast(this->_dsp));
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

    if(refreshCompander)
    {
        updateCompander(config);
    }

    if(refreshStereoWide)
    {
        updateStereoWide(config);
    }

#ifdef DEBUG_FPE
    fedisableexcept(FE_ALL_EXCEPT & ~FE_INEXACT & ~FE_INVALID);
#endif
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
        util::warning("Liveprog enable switch unset. Disabling liveprog.");
        enabled = false;
    }

    if(!propExists)
    {
        util::warning("liveprog_file property not found in cache. Disabling liveprog.");
        enabled = false;
    }

    // Attach log listener
    setStdOutHandler(receiveLiveprogStdOut, this);

    QFile f(file);
    if(!f.exists())
    {
        util::warning("Referenced file does not exist anymore. Disabling liveprog.");
        enabled = false;
    }

    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        util::error("Cannot open file path. Disabling liveprog.");
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
        util::warning("Syntax error in script file, cannot load. Reason: " + std::string(errorString));
    }
    if(ret <= 0)
    {
        util::warning("" + std::string(checkErrorCode(ret)));
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

std::vector<EelVariable> DspHost::enumEelVariables()
{
    std::vector<EelVariable> vars;

    compileContext *ctx = (compileContext*)cast(this->_dsp)->eel.vm;
    for (int i = 0; i < ctx->varTable_numBlocks; i++)
    {
        for (int j = 0; j < NSEEL_VARS_PER_BLOCK; j++)
        {
            EelVariable var;
            // char *valid = (char*)GetStringForIndex(ctx->region_context, ctx->varTable_Values[i][j], 0);
            // TODO fix string handling (broke after last libjamesdsp update)
            var.isString = false; // = valid;

            if (ctx->varTable_Names[i][j])
            {
                var.name = ctx->varTable_Names[i][j];
                /*if(var.isString)
                    var.value = valid;
                else*/
                var.value = ctx->varTable_Values[i][j];

                vars.push_back(var);
            }
        }
    }

    return vars;
}

bool DspHost::manipulateEelVariable(const char* name, float value)
{
    compileContext *ctx = (compileContext*)cast(this->_dsp)->eel.vm;
    for (int i = 0; i < ctx->varTable_numBlocks; i++)
    {
        for (int j = 0; j < NSEEL_VARS_PER_BLOCK; j++)
        {
            if(!ctx->varTable_Names[i][j] || std::strcmp(ctx->varTable_Names[i][j], name) != 0)
            {
                continue;
            }

            // TODO fix string handling & detection
            char *validString = nullptr;//(char*)GetStringForIndex(ctx->region_context, ctx->varTable_Values[i][j], 1);
            if(validString)
            {
                Log::error(QString("variable '%1' is a string; currently only numerical variables can be manipulated").arg(name));
                return false;
            }

            ctx->varTable_Values[i][j] = value;
            return true;
        }
    }

    Log::error(QString("variable '%1' not found").arg(name));
    return false;
}

void DspHost::freezeLiveprogExecution(bool freeze)
{
    cast(this->_dsp)->eel.active = !freeze;
    Log::debug("Liveprog execution has been " + (freeze ? QString("frozen") : "resumed"));
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

