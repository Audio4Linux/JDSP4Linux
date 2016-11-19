/*#define TAG "Equalizer16Band"
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG,__VA_ARGS__)*/
#include "EffectEqualizer.h"

//#include <math.h>

#define NUM_BANDS 12

/*      EQ presets      */
static int16_t gPresetAcoustic[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetBassBooster[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetBassReducer[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetClassical[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetDeep[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetFlat[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetRnB[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetRock[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetSmallSpeakers[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetTrebleBooster[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetTrebleReducer[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int16_t gPresetVocalBooster[NUM_BANDS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

struct sPresetConfig {
    const char * name;
    const int16_t * bandConfigs;
};

static const sPresetConfig gEqualizerPresets[] = {
    { "Acoustic",       gPresetAcoustic      },
    { "Bass Booster",   gPresetBassBooster   },
    { "Bass Reducer",   gPresetBassReducer   },
    { "Classical",      gPresetClassical     },
    { "Deep",           gPresetDeep          },
    { "Flat",           gPresetFlat          },
    { "R&B",            gPresetRnB           },
    { "Rock",           gPresetRock          },
    { "Small Speakers", gPresetSmallSpeakers },
    { "Treble Booster", gPresetTrebleBooster },
    { "Treble Reducer", gPresetTrebleReducer },
    { "Vocal Booster",  gPresetVocalBooster  }
};

static const int16_t gNumPresets = sizeof(gEqualizerPresets)/sizeof(sPresetConfig);
static int16_t mCurPreset = 0;

/*      End of EQ presets      */

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int16_t data;
} reply1x4_1x2_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int16_t data1;
    int16_t data2;
} reply1x4_2x2_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int32_t arg;
    int16_t data;
} reply2x4_1x2_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int32_t arg;
    int32_t data;
} reply2x4_1x4_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int32_t arg;
    int32_t data1;
    int32_t data2;
} reply2x4_2x4_t;

typedef struct {
    int32_t status;
    uint32_t psize;
    uint32_t vsize;
    int32_t cmd;
    int16_t data[14]; // numbands (12) + 2
} reply1x4_props_t;

EffectEqualizer::EffectEqualizer()
    : mFade(0)
{
    for (int32_t i = 0; i < 12; i++) {
        mBand[i] = 0;
    }
}

int32_t EffectEqualizer::command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData)
{
    if (cmdCode == EFFECT_CMD_SET_CONFIG) {
        int32_t ret = Effect::configure(pCmdData);
        if (ret != 0) {
            int32_t *replyData = (int32_t *) pReplyData;
            *replyData = ret;
            return 0;
        }

        int32_t *replyData = (int32_t *) pReplyData;
        *replyData = 0;
        return 0;
    }

    if (cmdCode == EFFECT_CMD_GET_PARAM) {
        effect_param_t *cep = (effect_param_t *) pCmdData;
        if (cep->psize == 4) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == EQ_PARAM_NUM_BANDS) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = NUM_BANDS;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_LEVEL_RANGE) {
                reply1x4_2x2_t *replyData = (reply1x4_2x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data1 = -2200;
                replyData->data2 = 2200;
                *replySize = sizeof(reply1x4_2x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_GET_NUM_OF_PRESETS) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = gNumPresets;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_PROPERTIES) {
                reply1x4_props_t *replyData = (reply1x4_props_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2*8;
                replyData->data[0] = (int16_t)-1; // PRESET_CUSTOM
                replyData->data[1] = (int16_t)12;  // number of bands
                for (int i = 0; i < NUM_BANDS; i++) {
                    replyData->data[2 + i] = (int16_t)(mBand[i] * 120 + 0.5f); // band levels
                }
                *replySize = sizeof(reply1x4_props_t);
                return 0;
            }
            if (cmd == EQ_PARAM_PREAMP_STRENGTH) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = mPreAmp;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_CUR_PRESET) {
                reply1x4_1x2_t *replyData = (reply1x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = mCurPreset;
                *replySize = sizeof(reply1x4_1x2_t);
                return 0;
            }
        } else if (cep->psize == 8) {
            int32_t cmd = ((int32_t *) cep)[3];
            int32_t arg = ((int32_t *) cep)[4];
            if (cmd == EQ_PARAM_BAND_LEVEL && arg >= 0 && arg < NUM_BANDS) {
                reply2x4_1x2_t *replyData = (reply2x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = int16_t(mBand[arg] * 120 + 0.5f);
                *replySize = sizeof(reply2x4_1x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_CENTER_FREQ && arg >= 0 && arg < NUM_BANDS) {
                if(arg == 0)
                {
                float centerFrequency = 32.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 1)
                {
                float centerFrequency = 64.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 2)
                {
                float centerFrequency = 126.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 3)
                {
                float centerFrequency = 220.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 4)
                {
                float centerFrequency = 360.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 5)
                {
                float centerFrequency = 700.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 6)
                {
                float centerFrequency = 1600.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 7)
                {
                float centerFrequency = 3200.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 8)
                {
                float centerFrequency = 4800.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 9)
                {
                float centerFrequency = 7000.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
		else if(arg == 10)
                {
                float centerFrequency = 10000.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else if(arg == 11)
                {
                float centerFrequency = 15000.0f;
                reply2x4_1x4_t *replyData = (reply2x4_1x4_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 4;
                replyData->data = int32_t(centerFrequency * 1000);
                *replySize = sizeof(reply2x4_1x4_t);
                }
                else
                {
                // Should not happen
                }
                return 0;
            }
            if (cmd == EQ_PARAM_GET_BAND) {
		int16_t band = 12;
                reply2x4_1x2_t *replyData = (reply2x4_1x2_t *) pReplyData;
                replyData->status = 0;
                replyData->vsize = 2;
                replyData->data = int16_t(band);
                *replySize = sizeof(reply2x4_1x2_t);
                return 0;
            }
            if (cmd == EQ_PARAM_GET_PRESET_NAME && arg >= 0 && arg < int32_t(gNumPresets)) {
                effect_param_t *replyData = (effect_param_t *)pReplyData;
                memcpy(pReplyData, pCmdData, sizeof(effect_param_t) + cep->psize);
                uint32_t *pValueSize = &replyData->vsize;
                int voffset = ((replyData->psize - 1) / sizeof(int32_t) + 1) * sizeof(int32_t);
                void *pValue = replyData->data + voffset;

                char *name = (char *)pValue;
                strncpy(name, gEqualizerPresets[arg].name, *pValueSize - 1);
                name[*pValueSize - 1] = 0;
                *pValueSize = strlen(name) + 1;
                *replySize = sizeof(effect_param_t) + voffset + replyData->vsize;
                replyData->status = 0;
                return 0;
            }
        }

        effect_param_t *replyData = (effect_param_t *) pReplyData;
        replyData->status = -EINVAL;
        replyData->vsize = 0;
        *replySize = sizeof(effect_param_t);
        return 0;
    }

    if (cmdCode == EFFECT_CMD_SET_PARAM) {
        effect_param_t *cep = (effect_param_t *) pCmdData;
        int32_t *replyData = (int32_t *) pReplyData;

        if (cep->psize == 4 && cep->vsize == 2) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == CUSTOM_EQ_PARAM_LOUDNESS_CORRECTION) {
                int16_t value = ((int16_t *) cep)[8];
//                mLoudnessAdjustment = value / 100.0f;
                *replyData = 0;
                return 0;
            }
        }

        if (cep->psize == 4 && cep->vsize == 2) {
            int32_t cmd = *((int32_t *) cep->data);
            int16_t arg = *((int16_t *) (cep->data + sizeof(int32_t)));

            if (cmd == EQ_PARAM_CUR_PRESET && arg >= 0 && arg < gNumPresets) {
                int16_t i = 0;
                for (i = 0; i < NUM_BANDS; i++) {
                    mBand[i] = gEqualizerPresets[arg].bandConfigs[i] / 100.0f;
                }
                mCurPreset = arg;
                *replyData = 0;
                refreshBands();
                return 0;
            } else {
            }
        }

        if (cep->psize == 8 && cep->vsize == 2) {
            int32_t cmd = ((int32_t *) cep)[3];
            int32_t arg = ((int32_t *) cep)[4];

            if (cmd == EQ_PARAM_BAND_LEVEL && arg >= 0 && arg < NUM_BANDS) {
                *replyData = 0;
                int16_t value = ((int16_t *) cep)[10];
                mBand[arg] = value / 100.0f;
//                //LOGI("Band number: %d, gain value:%f",arg+1,mBand[arg]);
                refreshBands();
                return 0;
            }
        }

        if (cep->psize == 4 && cep->vsize >= 4 && cep->vsize <= 16) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == EQ_PARAM_PROPERTIES) {
                *replyData = 0;
                if ((((int16_t *) cep)[8]) >= 0) {
                    *replyData = -EINVAL;
                    return 0;
                }
                if ((((int16_t *) cep)[9]) != NUM_BANDS) {
                    *replyData = -EINVAL;
                    return 0;
                }
                for (int i = 0; i < NUM_BANDS; i++) {
                    mBand[i] = ((int16_t *) cep)[10 + i] / 100.0f;
                }

                return 0;
            }
        }
        if (cep->psize == 4 && cep->vsize == 2) {
            int32_t cmd = ((int32_t *) cep)[3];
            if (cmd == EQ_PARAM_PREAMP_STRENGTH) {
                mPreAmp = ((int16_t *) cep)[8];
                int32_t *replyData = (int32_t *) pReplyData;
                *replyData = 0;
                return 0;
            }
        }
        *replyData = -EINVAL;
        return 0;
    }

    return Effect::command(cmdCode, cmdSize, pCmdData, replySize, pReplyData);
}

void EffectEqualizer::refreshBands()
{
            Iir::Butterworth::LowShelf<6> ls;
            ls.setup (6, mSamplingRate, 32.0, (mBand[0]*0.9-(mBand[6]*0.09+mBand[7]*0.09+mBand[8]*0.04+mBand[9]*0.04)));
            mSOS1Band1L.setSOS(ls[0].getA0(), ls[0].getA1(), ls[0].getA2(), ls[0].getB0(), ls[0].getB1(), ls[0].getB2());
            mSOS1Band1R.setSOS(ls[0].getA0(), ls[0].getA1(), ls[0].getA2(), ls[0].getB0(), ls[0].getB1(), ls[0].getB2());
            mSOS2Band1L.setSOS(ls[1].getA0(), ls[1].getA1(), ls[1].getA2(), ls[1].getB0(), ls[1].getB1(), ls[1].getB2());
            mSOS2Band1R.setSOS(ls[1].getA0(), ls[1].getA1(), ls[1].getA2(), ls[1].getB0(), ls[1].getB1(), ls[1].getB2());
            mSOS3Band1L.setSOS(ls[2].getA0(), ls[2].getA1(), ls[2].getA2(), ls[2].getB0(), ls[2].getB1(), ls[2].getB2());
            mSOS3Band1R.setSOS(ls[2].getA0(), ls[2].getA1(), ls[2].getA2(), ls[2].getB0(), ls[2].getB1(), ls[2].getB2());
            Iir::Butterworth::BandShelf<6> bs1;
            bs1.setup (6, mSamplingRate, 64.0f, 50.0, (mBand[1]*0.95-(mBand[6]*0.09+mBand[7]*0.09+mBand[8]*0.04+mBand[9]*0.04)));
            mSOS1Band2L.setSOS(bs1[0].getA0(), bs1[0].getA1(), bs1[0].getA2(), bs1[0].getB0(), bs1[0].getB1(), bs1[0].getB2());
            mSOS1Band2R.setSOS(bs1[0].getA0(), bs1[0].getA1(), bs1[0].getA2(), bs1[0].getB0(), bs1[0].getB1(), bs1[0].getB2());
            mSOS2Band2L.setSOS(bs1[1].getA0(), bs1[1].getA1(), bs1[1].getA2(), bs1[1].getB0(), bs1[1].getB1(), bs1[1].getB2());
            mSOS2Band2R.setSOS(bs1[1].getA0(), bs1[1].getA1(), bs1[1].getA2(), bs1[1].getB0(), bs1[1].getB1(), bs1[1].getB2());
            mSOS3Band3L.setSOS(bs1[2].getA0(), bs1[2].getA1(), bs1[2].getA2(), bs1[2].getB0(), bs1[2].getB1(), bs1[2].getB2());
            mSOS3Band3R.setSOS(bs1[2].getA0(), bs1[2].getA1(), bs1[2].getA2(), bs1[2].getB0(), bs1[2].getB1(), bs1[2].getB2());
            Iir::Butterworth::BandShelf<6> bs2;
            bs2.setup (6, mSamplingRate, 126.0f, 65.0, (mBand[2]*0.95-(mBand[6]*0.09+mBand[7]*0.09+mBand[8]*0.04+mBand[9]*0.04)));
            mSOS1Band3L.setSOS(bs2[0].getA0(), bs2[0].getA1(), bs2[0].getA2(), bs2[0].getB0(), bs2[0].getB1(), bs2[0].getB2());
            mSOS1Band3R.setSOS(bs2[0].getA0(), bs2[0].getA1(), bs2[0].getA2(), bs2[0].getB0(), bs2[0].getB1(), bs2[0].getB2());
            mSOS2Band3L.setSOS(bs2[1].getA0(), bs2[1].getA1(), bs2[1].getA2(), bs2[1].getB0(), bs2[1].getB1(), bs2[1].getB2());
            mSOS2Band3R.setSOS(bs2[1].getA0(), bs2[1].getA1(), bs2[1].getA2(), bs2[1].getB0(), bs2[1].getB1(), bs2[1].getB2());
            mSOS3Band3L.setSOS(bs2[2].getA0(), bs2[2].getA1(), bs2[2].getA2(), bs2[2].getB0(), bs2[2].getB1(), bs2[2].getB2());
            mSOS3Band3R.setSOS(bs2[2].getA0(), bs2[2].getA1(), bs2[2].getA2(), bs2[2].getB0(), bs2[2].getB1(), bs2[2].getB2());
            Iir::Butterworth::BandShelf<4> bs3;
            bs3.setup (4, mSamplingRate, 220.0f, 120.0, (mBand[3]*0.95-(mBand[6]*0.09+mBand[7]*0.09+mBand[8]*0.04+mBand[9]*0.04)));
            mSOS1Band4L.setSOS(bs3[0].getA0(), bs3[0].getA1(), bs3[0].getA2(), bs3[0].getB0(), bs3[0].getB1(), bs3[0].getB2());
            mSOS1Band4R.setSOS(bs3[0].getA0(), bs3[0].getA1(), bs3[0].getA2(), bs3[0].getB0(), bs3[0].getB1(), bs3[0].getB2());
            mSOS2Band4L.setSOS(bs3[1].getA0(), bs3[1].getA1(), bs3[1].getA2(), bs3[1].getB0(), bs3[1].getB1(), bs3[1].getB2());
            mSOS2Band4R.setSOS(bs3[1].getA0(), bs3[1].getA1(), bs3[1].getA2(), bs3[1].getB0(), bs3[1].getB1(), bs3[1].getB2());
            Iir::Butterworth::BandShelf<4> bs4;
            bs4.setup (4, mSamplingRate, 380.0f, 180.0, (mBand[4]*0.98-(mBand[6]*0.09+mBand[7]*0.09+mBand[8]*0.04+mBand[9]*0.04)));
            mSOS1Band5L.setSOS(bs4[0].getA0(), bs4[0].getA1(), bs4[0].getA2(), bs4[0].getB0(), bs4[0].getB1(), bs4[0].getB2());
            mSOS1Band5R.setSOS(bs4[0].getA0(), bs4[0].getA1(), bs4[0].getA2(), bs4[0].getB0(), bs4[0].getB1(), bs4[0].getB2());
            mSOS2Band5L.setSOS(bs4[1].getA0(), bs4[1].getA1(), bs4[1].getA2(), bs4[1].getB0(), bs4[1].getB1(), bs4[1].getB2());
            mSOS2Band5R.setSOS(bs4[1].getA0(), bs4[1].getA1(), bs4[1].getA2(), bs4[1].getB0(), bs4[1].getB1(), bs4[1].getB2());
            Iir::Butterworth::BandShelf<6> bs5;
            bs5.setup (6, mSamplingRate, 750.0f, 600.0, (mBand[5]*0.99-(mBand[6]*0.09+mBand[7]*0.09+mBand[8]*0.04+mBand[9]*0.04)));
            mSOS1Band6L.setSOS(bs5[0].getA0(), bs5[0].getA1(), bs5[0].getA2(), bs5[0].getB0(), bs5[0].getB1(), bs5[0].getB2());
            mSOS1Band6R.setSOS(bs5[0].getA0(), bs5[0].getA1(), bs5[0].getA2(), bs5[0].getB0(), bs5[0].getB1(), bs5[0].getB2());
            mSOS2Band6L.setSOS(bs5[1].getA0(), bs5[1].getA1(), bs5[1].getA2(), bs5[1].getB0(), bs5[1].getB1(), bs5[1].getB2());
            mSOS2Band6R.setSOS(bs5[1].getA0(), bs5[1].getA1(), bs5[1].getA2(), bs5[1].getB0(), bs5[1].getB1(), bs5[1].getB2());
            mSOS3Band6L.setSOS(bs5[2].getA0(), bs5[2].getA1(), bs5[2].getA2(), bs5[2].getB0(), bs5[2].getB1(), bs5[2].getB2());
            mSOS3Band6R.setSOS(bs5[2].getA0(), bs5[2].getA1(), bs5[2].getA2(), bs5[2].getB0(), bs5[2].getB1(), bs5[2].getB2());
            Iir::Butterworth::BandShelf<6> bs6;
            bs6.setup (6, mSamplingRate, 1600.0f, 1000.0, mBand[6]*0.95);
            mSOS1Band7L.setSOS(bs6[0].getA0(), bs6[0].getA1(), bs6[0].getA2(), bs6[0].getB0(), bs6[0].getB1(), bs6[0].getB2());
            mSOS1Band7R.setSOS(bs6[0].getA0(), bs6[0].getA1(), bs6[0].getA2(), bs6[0].getB0(), bs6[0].getB1(), bs6[0].getB2());
            mSOS2Band7L.setSOS(bs6[1].getA0(), bs6[1].getA1(), bs6[1].getA2(), bs6[1].getB0(), bs6[1].getB1(), bs6[1].getB2());
            mSOS2Band7R.setSOS(bs6[1].getA0(), bs6[1].getA1(), bs6[1].getA2(), bs6[1].getB0(), bs6[1].getB1(), bs6[1].getB2());
            mSOS3Band7L.setSOS(bs6[2].getA0(), bs6[2].getA1(), bs6[2].getA2(), bs6[2].getB0(), bs6[2].getB1(), bs6[2].getB2());
            mSOS3Band7R.setSOS(bs6[2].getA0(), bs6[2].getA1(), bs6[2].getA2(), bs6[2].getB0(), bs6[2].getB1(), bs6[2].getB2());
            Iir::Butterworth::BandShelf<6> bs7;
            bs7.setup (6, mSamplingRate, 3000.0f, 1800.0, mBand[7]*0.95);
            mSOS1Band8L.setSOS(bs7[0].getA0(), bs7[0].getA1(), bs7[0].getA2(), bs7[0].getB0(), bs7[0].getB1(), bs7[0].getB2());
            mSOS1Band8R.setSOS(bs7[0].getA0(), bs7[0].getA1(), bs7[0].getA2(), bs7[0].getB0(), bs7[0].getB1(), bs7[0].getB2());
            mSOS2Band8L.setSOS(bs7[1].getA0(), bs7[1].getA1(), bs7[1].getA2(), bs7[1].getB0(), bs7[1].getB1(), bs7[1].getB2());
            mSOS2Band8R.setSOS(bs7[1].getA0(), bs7[1].getA1(), bs7[1].getA2(), bs7[1].getB0(), bs7[1].getB1(), bs7[1].getB2());
            mSOS3Band8L.setSOS(bs7[2].getA0(), bs7[2].getA1(), bs7[2].getA2(), bs7[2].getB0(), bs7[2].getB1(), bs7[2].getB2());
            mSOS3Band8R.setSOS(bs7[2].getA0(), bs7[2].getA1(), bs7[2].getA2(), bs7[2].getB0(), bs7[2].getB1(), bs7[2].getB2());
            Iir::Butterworth::BandShelf<10> bs8;
            bs8.setup (10, mSamplingRate, 4800.0f, 1800.0, mBand[8]);
            mSOS1Band9L.setSOS(bs8[0].getA0(), bs8[0].getA1(), bs8[0].getA2(), bs8[0].getB0(), bs8[0].getB1(), bs8[0].getB2());
            mSOS1Band9R.setSOS(bs8[0].getA0(), bs8[0].getA1(), bs8[0].getA2(), bs8[0].getB0(), bs8[0].getB1(), bs8[0].getB2());
            mSOS2Band9L.setSOS(bs8[1].getA0(), bs8[1].getA1(), bs8[1].getA2(), bs8[1].getB0(), bs8[1].getB1(), bs8[1].getB2());
            mSOS2Band9R.setSOS(bs8[1].getA0(), bs8[1].getA1(), bs8[1].getA2(), bs8[1].getB0(), bs8[1].getB1(), bs8[1].getB2());
            mSOS3Band9L.setSOS(bs8[2].getA0(), bs8[2].getA1(), bs8[2].getA2(), bs8[2].getB0(), bs8[2].getB1(), bs8[2].getB2());
            mSOS3Band9R.setSOS(bs8[2].getA0(), bs8[2].getA1(), bs8[2].getA2(), bs8[2].getB0(), bs8[2].getB1(), bs8[2].getB2());
            mSOS4Band9L.setSOS(bs8[3].getA0(), bs8[3].getA1(), bs8[3].getA2(), bs8[3].getB0(), bs8[3].getB1(), bs8[3].getB2());
            mSOS4Band9R.setSOS(bs8[3].getA0(), bs8[3].getA1(), bs8[3].getA2(), bs8[3].getB0(), bs8[3].getB1(), bs8[3].getB2());
            mSOS5Band9L.setSOS(bs8[4].getA0(), bs8[4].getA1(), bs8[4].getA2(), bs8[4].getB0(), bs8[4].getB1(), bs8[4].getB2());
            mSOS5Band9R.setSOS(bs8[4].getA0(), bs8[4].getA1(), bs8[4].getA2(), bs8[4].getB0(), bs8[4].getB1(), bs8[4].getB2());
            Iir::Butterworth::BandShelf<8> bs9;
            bs9.setup (8, mSamplingRate, 7000.0f, 2500.0, mBand[9]*0.95);
            mSOS1Band10L.setSOS(bs9[0].getA0(), bs9[0].getA1(), bs9[0].getA2(), bs9[0].getB0(), bs9[0].getB1(), bs9[0].getB2());
            mSOS1Band10R.setSOS(bs9[0].getA0(), bs9[0].getA1(), bs9[0].getA2(), bs9[0].getB0(), bs9[0].getB1(), bs9[0].getB2());
            mSOS2Band10L.setSOS(bs9[1].getA0(), bs9[1].getA1(), bs9[1].getA2(), bs9[1].getB0(), bs9[1].getB1(), bs9[1].getB2());
            mSOS2Band10R.setSOS(bs9[1].getA0(), bs9[1].getA1(), bs9[1].getA2(), bs9[1].getB0(), bs9[1].getB1(), bs9[1].getB2());
            mSOS3Band10L.setSOS(bs9[2].getA0(), bs9[2].getA1(), bs9[2].getA2(), bs9[2].getB0(), bs9[2].getB1(), bs9[2].getB2());
            mSOS3Band10R.setSOS(bs9[2].getA0(), bs9[2].getA1(), bs9[2].getA2(), bs9[2].getB0(), bs9[2].getB1(), bs9[2].getB2());
            mSOS4Band10L.setSOS(bs9[3].getA0(), bs9[3].getA1(), bs9[3].getA2(), bs9[3].getB0(), bs9[3].getB1(), bs9[3].getB2());
            mSOS4Band10R.setSOS(bs9[3].getA0(), bs9[3].getA1(), bs9[3].getA2(), bs9[3].getB0(), bs9[3].getB1(), bs9[3].getB2());
            mPeakFilter11L.setPeaking(11500.0f, mSamplingRate, mBand[10]*0.8, 1.8f);
            mPeakFilter11R.setPeaking(11500.0f, mSamplingRate, mBand[10]*0.8, 1.8f);
            mHSFilter12L.setHighShelf(16000.0f, mSamplingRate, mBand[11], 1.0f);
            mHSFilter12R.setHighShelf(16000.0f, mSamplingRate, mBand[11], 1.0f);
}
int32_t EffectEqualizer::process(audio_buffer_t *in, audio_buffer_t *out)
{
    for (uint32_t i = 0; i < in->frameCount; i ++) {
        int32_t dryL = read(in, i * 2);
        int32_t dryR = read(in, i * 2 + 1);
        dryL = mSOS1Band1L.process(dryL);
        dryL = mSOS2Band1L.process(dryL);
        dryL = mSOS3Band1L.process(dryL);
        dryR = mSOS1Band1R.process(dryR);
        dryR = mSOS2Band1R.process(dryR);
        dryR = mSOS3Band1R.process(dryR);
        dryL = mSOS1Band2L.process(dryL);
        dryL = mSOS2Band2L.process(dryL);
        dryL = mSOS3Band2L.process(dryL);
        dryR = mSOS1Band2R.process(dryR);
        dryR = mSOS2Band2R.process(dryR);
        dryR = mSOS3Band2R.process(dryR);
        dryL = mSOS1Band3L.process(dryL);
        dryL = mSOS2Band3L.process(dryL);
        dryL = mSOS3Band3L.process(dryL);
        dryR = mSOS1Band3R.process(dryR);
        dryR = mSOS2Band3R.process(dryR);
        dryR = mSOS3Band3R.process(dryR);
        dryL = mSOS1Band4L.process(dryL);
        dryL = mSOS2Band4L.process(dryL);
        dryR = mSOS1Band4R.process(dryR);
        dryR = mSOS2Band4R.process(dryR);
        dryL = mSOS1Band5L.process(dryL);
        dryL = mSOS2Band5L.process(dryL);
        dryR = mSOS1Band5R.process(dryR);
        dryR = mSOS2Band5R.process(dryR);
        dryL = mSOS1Band6L.process(dryL);
        dryL = mSOS2Band6L.process(dryL);
        dryL = mSOS3Band6L.process(dryL);
        dryR = mSOS1Band6R.process(dryR);
        dryR = mSOS2Band6R.process(dryR);
        dryR = mSOS3Band6R.process(dryR);
        dryL = mSOS1Band7L.process(dryL);
        dryL = mSOS2Band7L.process(dryL);
        dryL = mSOS3Band7L.process(dryL);
        dryR = mSOS1Band7R.process(dryR);
        dryR = mSOS2Band7R.process(dryR);
        dryR = mSOS3Band7R.process(dryR);
        dryL = mSOS1Band8L.process(dryL);
        dryL = mSOS2Band8L.process(dryL);
        dryL = mSOS3Band8L.process(dryL);
        dryR = mSOS1Band8R.process(dryR);
        dryR = mSOS2Band8R.process(dryR);
        dryR = mSOS3Band8R.process(dryR);
        dryL = mSOS1Band9L.process(dryL);
        dryL = mSOS2Band9L.process(dryL);
        dryL = mSOS3Band9L.process(dryL);
        dryL = mSOS4Band9L.process(dryL);
        dryL = mSOS5Band9L.process(dryL);
        dryR = mSOS1Band9R.process(dryR);
        dryR = mSOS2Band9R.process(dryR);
        dryR = mSOS3Band9R.process(dryR);
        dryR = mSOS4Band9R.process(dryR);
        dryR = mSOS5Band9R.process(dryR);
        dryL = mSOS1Band10L.process(dryL);
        dryL = mSOS2Band10L.process(dryL);
        dryL = mSOS3Band10L.process(dryL);
        dryL = mSOS4Band10L.process(dryL);
        dryR = mSOS1Band10R.process(dryR);
        dryR = mSOS2Band10R.process(dryR);
        dryR = mSOS3Band10R.process(dryR);
        dryR = mSOS4Band10R.process(dryR);
        dryL = mPeakFilter11L.process(dryL);
        dryR = mPeakFilter11R.process(dryR);
        dryL = mHSFilter12L.process(dryL);
        dryR = mHSFilter12R.process(dryR);
        write(out, i * 2, dryL);
        write(out, i * 2 + 1, dryR);
    }
		return mEnable || mFade != 0 ? 0 : -ENODATA;
}
