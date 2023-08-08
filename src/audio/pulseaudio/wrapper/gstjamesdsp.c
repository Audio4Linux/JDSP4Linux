#include "gstjamesdsp.h"

#include <stdio.h>
#include <string.h>
#include <gst/base/base.h>
#include <gst/base/gstbasetransform.h>
#include <gst/controller/controller.h>
#include <math.h>

GST_DEBUG_CATEGORY_STATIC (gst_jamesdsp_debug);
#define GST_CAT_DEFAULT gst_jamesdsp_debug
/* Filter signals and args */
enum {
    /* FILL ME */
    LAST_SIGNAL
};

enum {
    PROP_0,

    /* Read-write */
    PROP_DSP_ENABLE,
    /* Read-only */
    PROP_DSP_PTR,
    PROP_DSP_SRATE,
    PROP_DSP_FORMAT
};

#define gst_jamesdsp_parent_class parent_class

G_DEFINE_TYPE (Gstjamesdsp, gst_jamesdsp, GST_TYPE_AUDIO_FILTER);

static void gst_jamesdsp_set_property(GObject *object, guint prop_id,
                                      const GValue *value, GParamSpec *pspec);

static void gst_jamesdsp_get_property(GObject *object, guint prop_id,
                                      GValue *value, GParamSpec *pspec);

static void gst_jamesdsp_finalize(GObject *object);

static gboolean gst_jamesdsp_setup(GstAudioFilter *self,
                                   const GstAudioInfo *info);

static gboolean gst_jamesdsp_stop(GstBaseTransform *base);

static GstFlowReturn gst_jamesdsp_transform_ip(GstBaseTransform *base,
                                               GstBuffer *outbuf);

/* GObject vmethod implementations */

/* initialize the jamesdsp's class */
static void
gst_jamesdsp_class_init(GstjamesdspClass *klass) {

    GObjectClass *gobject_class = (GObjectClass *) klass;
    GstElementClass *gstelement_class = (GstElementClass *) klass;
    GstBaseTransformClass *basetransform_class = (GstBaseTransformClass *) klass;
    GstAudioFilterClass *audioself_class = (GstAudioFilterClass *) klass;
    GstCaps *caps;

    /* debug category for fltering log messages
     */
    GST_DEBUG_CATEGORY_INIT (gst_jamesdsp_debug, "jamesdsp", 0, "jamesdsp element");

    gobject_class->set_property = gst_jamesdsp_set_property;
    gobject_class->get_property = gst_jamesdsp_get_property;
    gobject_class->finalize = gst_jamesdsp_finalize;

    /* global switch */
    g_object_class_install_property(gobject_class, PROP_DSP_ENABLE,
                                    g_param_spec_boolean("dsp_enable", "Enabled", "Enable processing",
                                                         FALSE, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE));

    g_object_class_install_property(gobject_class, PROP_DSP_PTR,
                                    g_param_spec_pointer("dsp_ptr", "DspObjectPointer", "JamesDSP class object pointer",
                                                         G_PARAM_READABLE));

    g_object_class_install_property(gobject_class, PROP_DSP_SRATE,
                                    g_param_spec_string("dsp_srate", "StreamSamplingRate", "Current sampling rate",
                                                     "0", G_PARAM_READABLE));

    g_object_class_install_property(gobject_class, PROP_DSP_FORMAT,
                                    g_param_spec_string("dsp_format", "StreamFormat", "Current audio format",
                                                        "Unknown", G_PARAM_READABLE));

    gst_element_class_set_static_metadata(gstelement_class,
                                          "jamesdsp",
                                          "Filter/Effect/Audio",
                                          "GStreamer1 wrapper for libjamesdsp (@james34602)",
                                          "Tim Schneeberger <tim.schneeberger@outlook.de>");

    caps = gst_caps_from_string(ALLOWED_CAPS);
    gst_audio_filter_class_add_pad_templates(GST_JAMESDSP_CLASS (klass), caps);
    gst_caps_unref(caps);

    audioself_class->setup = GST_DEBUG_FUNCPTR (gst_jamesdsp_setup);
    basetransform_class->transform_ip =
            GST_DEBUG_FUNCPTR (gst_jamesdsp_transform_ip);
    basetransform_class->transform_ip_on_passthrough = FALSE;
    basetransform_class->stop = GST_DEBUG_FUNCPTR (gst_jamesdsp_stop);
}

/* initialize the new element
 * allocate private resources
 */
static void
gst_jamesdsp_init(Gstjamesdsp *self) {
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM (self), TRUE);
    gst_base_transform_set_gap_aware(GST_BASE_TRANSFORM (self), TRUE);

    /* initialize properties */
    self->dsp = malloc(sizeof(JamesDSPLib));
    memset(self->dsp, 0, sizeof(JamesDSPLib));

    // TODO: Add benchmark toggle support to pulse version...
    JamesDSPGlobalMemoryAllocation();
    JamesDSPInit(self->dsp, 128, 48000);

    self->enable = FALSE;
    self->samplerate = 48000;
    self->format = -1;

    g_mutex_init(&self->lock);
}

/* free private resources
*/
static void
gst_jamesdsp_finalize(GObject *object) {
    Gstjamesdsp *self = GST_JAMESDSP (object);

    self->enable = FALSE;

    JamesDSPFree(self->dsp);
    self->dsp = NULL;

    JamesDSPGlobalMemoryDeallocation();
    g_mutex_clear(&self->lock);

    G_OBJECT_CLASS (parent_class)->finalize(object);
}

static void
gst_jamesdsp_set_property(GObject *object, guint prop_id,
                          const GValue *value, GParamSpec *pspec) {
    Gstjamesdsp *self = GST_JAMESDSP (object);

    switch (prop_id) {
        case PROP_DSP_ENABLE:
            g_mutex_lock(&self->lock);
            self->enable = g_value_get_boolean(value);
            g_mutex_unlock(&self->lock);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void
gst_jamesdsp_get_property(GObject *object, guint prop_id,
                          GValue *value, GParamSpec *pspec) {
    Gstjamesdsp *self = GST_JAMESDSP (object);

    switch (prop_id) {
        case PROP_DSP_ENABLE:
            g_value_set_boolean(value, self->enable);
            break;
        case PROP_DSP_PTR:
            g_value_set_pointer(value, self->dsp);
            break;
        case PROP_DSP_SRATE: {
            char str[8];
            snprintf(str, 8, "%d", self->samplerate);
            g_value_set_string(value, str);
            break;
        }
        case PROP_DSP_FORMAT:
            switch(self->format){
                case 0:
                    g_value_set_string(value, "16-bit signed samples, little endian");
                    break;
                case 1:
                    g_value_set_string(value, "32-bit signed samples, little endian");
                    break;
                case 2:
                    g_value_set_string(value, "32-bit floating point samples, little endian");
                    break;
                default:
                    g_value_set_string(value, "Unknown");
                    break;
            }
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }

}

/* GstBaseTransform vmethod implementations */

static gboolean
gst_jamesdsp_setup(GstAudioFilter *base, const GstAudioInfo *info) {
    Gstjamesdsp *self = GST_JAMESDSP (base);

    g_mutex_lock(&self->lock);
    if (info) {
        self->samplerate = GST_AUDIO_INFO_RATE (info);
    } else {
        self->samplerate = GST_AUDIO_FILTER_RATE (self);
    }
    g_mutex_unlock(&self->lock);

    if (self->samplerate <= 0)
    {
        GST_WARNING_OBJECT (self, "current sample_rate < 1");
        return FALSE;
    }

    GST_DEBUG_OBJECT (self, "current sample_rate = %d", self->samplerate);

    JamesDSPSetSampleRate(self->dsp, self->samplerate, 0);

    return TRUE;
}

static gboolean
gst_jamesdsp_stop(GstBaseTransform *base) {
    Gstjamesdsp *self = GST_JAMESDSP (base);
    return TRUE;
}

/* this function does the actual processing
 */
static GstFlowReturn
gst_jamesdsp_transform_ip(GstBaseTransform *base, GstBuffer *buf) {
    Gstjamesdsp *filter = GST_JAMESDSP (base);
    guint idx, num_samples;
    float *pcm_data;
    GstClockTime timestamp, stream_time;
    GstMapInfo map;

    timestamp = GST_BUFFER_TIMESTAMP (buf);
    stream_time =
            gst_segment_to_stream_time(&base->segment, GST_FORMAT_TIME, timestamp);

    if (GST_CLOCK_TIME_IS_VALID (stream_time))
        gst_object_sync_values(GST_OBJECT (filter), stream_time);

    if (G_UNLIKELY (GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_GAP)))
        return GST_FLOW_OK;

    if (filter->enable)
    {
        gst_buffer_map(buf, &map, GST_MAP_READWRITE);
        num_samples = map.size / GST_AUDIO_FILTER_BPS (filter) / 2;
        pcm_data = (float *) (map.data);

        g_mutex_lock(&filter->lock);

        GstStructure *stru;
        stru = gst_caps_get_structure (gst_pad_get_current_caps (base->sinkpad), 0);
        if(strstr(gst_structure_get_string(stru, "format"),"S16LE")!=NULL)
        {            
            filter->format = 0;

            int16_t* data = (int16_t*)map.data;
            int16_t* out = malloc(2 * num_samples * sizeof(int16_t));

            filter->dsp->processInt16Multiplexd(filter->dsp, data, out, num_samples);
            for(uint32_t i = 0; i < num_samples * 2; i++)
            {
                data[i] = out[i];
            }

            free(out);
        }
        else if(strstr(gst_structure_get_string(stru, "format"),"S32LE")!=NULL)
        {
            filter->format = 1;

            int32_t* data = (int32_t*)map.data;
            int32_t* out = malloc(2 * num_samples * sizeof(int32_t));

            filter->dsp->processInt32Multiplexd(filter->dsp, data, out, num_samples);
            for(uint32_t i = 0; i < num_samples * 2; i++)
            {
                data[i] = out[i];
            }

            free(out);
        }
        else if(strstr(gst_structure_get_string(stru, "format"),"F32LE")!=NULL)
        {
            filter->format = 2;

            float* data = (float*)map.data;
            float* out = malloc(2 * num_samples * sizeof(float));

            filter->dsp->processFloatMultiplexd(filter->dsp, data, out, num_samples);
            for(uint32_t i = 0; i < num_samples * 2; i++)
            {
                data[i] = out[i];
            }

            free(out);
        }

        g_mutex_unlock(&filter->lock);
        gst_buffer_unmap(buf, &map);
    }

    return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
gboolean jamesdsp_init(GstPlugin *jamesdsp) {
    return gst_element_register(jamesdsp, "jamesdsp", GST_RANK_NONE,
                                GST_TYPE_JAMESDSP);
}


