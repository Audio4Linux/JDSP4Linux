#pragma once

#include "Biquad4proc.h"
#include "Effect.h"

class EffectCompression : public Effect {
    private:
    int32_t mUserLevel[2];
    float mCompressionRatio;

    int32_t mFade;
    int32_t mCurrentLevel[2];

    Biquad4proc mWeigherBP[2];

    uint64_t estimateOneChannelLevel(audio_buffer_t *in, int32_t interleave, int32_t offset, Biquad4proc& WeigherBP);

    public:
    EffectCompression();
    int32_t command(uint32_t cmdCode, uint32_t cmdSize, void* pCmdData, uint32_t* replySize, void* pReplyData);
    int32_t process(audio_buffer_t *in, audio_buffer_t *out);
};
