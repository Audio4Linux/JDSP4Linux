LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := jamesDSPImpulseToolbox
LOCAL_SRC_FILES := \
    libsndfile/G72x/g721.c libsndfile/G72x/g723_16.c libsndfile/G72x/g723_24.c libsndfile/G72x/g723_40.c libsndfile/G72x/g72x.c \
    libsndfile/GSM610/add.c libsndfile/GSM610/code.c libsndfile/GSM610/decode.c libsndfile/GSM610/gsm_create.c libsndfile/GSM610/gsm_decode.c \
    libsndfile/GSM610/gsm_destroy.c libsndfile/GSM610/gsm_encode.c libsndfile/GSM610/gsm_option.c libsndfile/GSM610/long_term.c \
    libsndfile/GSM610/lpc.c libsndfile/GSM610/preprocess.c libsndfile/GSM610/rpe.c libsndfile/GSM610/short_term.c libsndfile/GSM610/table.c \
    libsndfile/common.c libsndfile/file_io.c libsndfile/command.c \
    libsndfile/pcm.c libsndfile/ulaw.c libsndfile/alaw.c libsndfile/float32.c libsndfile/double64.c libsndfile/ima_adpcm.c \
    libsndfile/ms_adpcm.c libsndfile/gsm610.c libsndfile/dwvw.c libsndfile/vox_adpcm.c libsndfile/interleave.c libsndfile/strings.c \
    libsndfile/dither.c libsndfile/broadcast.c libsndfile/audio_detect.c libsndfile/ima_oki_adpcm.c libsndfile/chunk.c libsndfile/ogg.c \
    libsndfile/chanmap.c libsndfile/windows.c libsndfile/id3.c libsndfile/sndfile.c libsndfile/aiff.c libsndfile/paf.c libsndfile/pvf.c \
    libsndfile/au.c libsndfile/avr.c libsndfile/caf.c libsndfile/dwd.c libsndfile/flac.c libsndfile/g72x.c libsndfile/htk.c \
    libsndfile/ircam.c libsndfile/macbinary3.c libsndfile/macos.c libsndfile/mat4.c libsndfile/mat5.c libsndfile/nist.c \
    libsndfile/raw.c libsndfile/rx2.c libsndfile/sd2.c libsndfile/sds.c libsndfile/svx.c libsndfile/txw.c libsndfile/voc.c \
    libsndfile/wve.c libsndfile/w64.c libsndfile/wav_w64.c libsndfile/wav.c libsndfile/xi.c libsndfile/mpc2k.c libsndfile/rf64.c \
    JdspImpResToolbox.c \

LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
LOCAL_LDFLAGS += -Wl,--gc-sections,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)