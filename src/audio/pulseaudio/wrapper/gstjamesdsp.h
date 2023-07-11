#ifndef GSTJAMESDSP_H
#define GSTJAMESDSP_H

#include <gst/gst.h>
#include <gst/audio/audio.h>

#ifdef DEBUG_FPE
#error DEBUG_FPE only implemented in the PipeWire version
#endif

G_BEGIN_DECLS

#include <jdsp_header.h>

#define GST_TYPE_JAMESDSP            (gst_jamesdsp_get_type())
#define GST_JAMESDSP(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_JAMESDSP,Gstjamesdsp))
#define GST_JAMESDSP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass) ,GST_TYPE_JAMESDSP,GstjamesdspClass))
#define GST_JAMESDSP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj) ,GST_TYPE_JAMESDSP,GstjamesdspClass))
#define GST_IS_JAMESDSP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_JAMESDSP))
#define GST_IS_JAMESDSP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass) ,GST_TYPE_JAMESDSP))

#define PACKAGE "jamesdsp-plugin"
#define VERSION "4.1.0"
#define ALLOWED_CAPS \
  "audio/x-raw,"                            \
  " format=(string){" GST_AUDIO_NE(F32) "," GST_AUDIO_NE(S32) "," GST_AUDIO_NE(S16) "},"  \
  " rate=(int)[44100,48000],"                 \
  " channels=(int)2,"                       \
  " layout=(string){ (string)interleaved }"

typedef struct _Gstjamesdsp     Gstjamesdsp;
typedef struct _GstjamesdspClass GstjamesdspClass;

struct _Gstjamesdsp {
    GstAudioFilter audiofilter;

    JamesDSPLib* dsp;
    gboolean enable;

    /* < private > */
    GMutex lock;
    int samplerate;
    int format;
};

struct _GstjamesdspClass {
    GstAudioFilterClass parent_class;
};

GType gst_jamesdsp_get_type (void);

extern gboolean jamesdsp_init(GstPlugin *jamesdsp);

G_END_DECLS


#endif // GSTJAMESDSP_H
