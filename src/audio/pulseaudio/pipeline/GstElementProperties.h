#ifndef GSTELEMENTPROPERTIES_H
#define GSTELEMENTPROPERTIES_H

#include <list>
#include <gst/gst.h>
#include <cassert>
#include <cstdint>

class GstElementProperties {
public:
    GstElementProperties(GstElement* _e, bool debug_output = true);

    std::list<GstStaticPadTemplate> getTemplates();
    gboolean canLinkPads (GstPad * srcpad, GstPad * sinkpad);
    gboolean hasTemplate (GstPadDirection direction);
    gboolean hasNonMonoTemplate(GstPadDirection direction);
    gboolean hasCompatibleCapsWith(GstElement* e2);
    gboolean isSometimesPadRequired (GstPadDirection direction);
    gboolean hasSometimesTemplate (GstPadDirection direction);
    gboolean hasAlwaysTemplate (GstPadDirection direction);
    gboolean hasOnlyUnsupportedPads (GstPadDirection direction);
    GstPadPresence* isLinkablePadAvailable (GstPadDirection direction);
    gboolean hasSpecificTemplate (GstPadPresence pres, GstPadDirection direction);
    void printPadTemplates ();
    void printPadsOfElement(GstElement* dest);

    static gboolean haveCompatibleCaps (GstPad * srcpad, GstPad * sinkpad)
    {
        /* generic checks */
        g_return_val_if_fail (GST_IS_PAD (srcpad), FALSE);
        g_return_val_if_fail (GST_IS_PAD (sinkpad), FALSE);

        GstPadLinkCheck flags = GST_PAD_LINK_CHECK_DEFAULT;
        GstCaps *srccaps = NULL;
        GstCaps *sinkcaps = NULL;
        gboolean compatible = FALSE;

        GST_OBJECT_LOCK (srcpad);
        GST_OBJECT_LOCK (sinkpad);

        if (!(flags & (GST_PAD_LINK_CHECK_CAPS | GST_PAD_LINK_CHECK_TEMPLATE_CAPS)))
            return TRUE;

        /* Doing the expensive caps checking takes priority over only checking the template caps */
        if (flags & GST_PAD_LINK_CHECK_CAPS) {
            GST_OBJECT_UNLOCK (sinkpad);
            GST_OBJECT_UNLOCK (srcpad);

            srccaps = gst_pad_query_caps (srcpad, NULL);
            sinkcaps = gst_pad_query_caps (sinkpad, NULL);

            GST_OBJECT_LOCK (srcpad);
            GST_OBJECT_LOCK (sinkpad);
        } else {
            /* If one of the two pads doesn't have a template, consider the intersection
       * as valid.*/
            if (G_UNLIKELY ((GST_PAD_PAD_TEMPLATE (srcpad) == NULL)
                            || (GST_PAD_PAD_TEMPLATE (sinkpad) == NULL))) {
                compatible = TRUE;
                goto done;
            }
            srccaps = gst_caps_ref (GST_PAD_TEMPLATE_CAPS (GST_PAD_PAD_TEMPLATE (srcpad)));
            sinkcaps =
                    gst_caps_ref (GST_PAD_TEMPLATE_CAPS (GST_PAD_PAD_TEMPLATE (sinkpad)));
        }

        /* if we have caps on both pads we can check the intersection. If one
     * of the caps is %NULL, we return %TRUE. */
        if (G_UNLIKELY (srccaps == NULL || sinkcaps == NULL)) {
            if (srccaps)
                gst_caps_unref (srccaps);
            if (sinkcaps)
                gst_caps_unref (sinkcaps);
            goto done;
        }

        compatible = gst_caps_can_intersect (srccaps, sinkcaps);
        gst_caps_unref (srccaps);
        gst_caps_unref (sinkcaps);

        GST_OBJECT_UNLOCK (sinkpad);
        GST_OBJECT_UNLOCK (srcpad);

done:
        return compatible;
    }

    static gboolean printField (GQuark field,
                                const GValue * value,
                                [[maybe_unused]] gpointer userdata)
    {

        gchar *str = gst_value_serialize (value);

        g_debug ("\t%15s: %s", g_quark_to_string (field), str);
        g_free (str);
        return TRUE;
    }

    static void printCaps (const GstCaps * caps)
    {
        guint i;

        g_return_if_fail (caps != NULL);

        if (gst_caps_is_any (caps)) {
            g_debug ("ANY");
            return;
        }
        if (gst_caps_is_empty (caps)) {
            g_debug ("EMPTY");
            return;
        }

        for (i = 0; i < gst_caps_get_size (caps); i++) {
            GstStructure *structure = gst_caps_get_structure (caps, i);
            GstCapsFeatures *features = gst_caps_get_features (caps, i);

            if (features && (gst_caps_features_is_any (features) ||
                             !gst_caps_features_is_equal (features,
                                                          GST_CAPS_FEATURES_MEMORY_SYSTEM_MEMORY))) {
                gchar *features_string = gst_caps_features_to_string (features);

                g_debug ("%s(%s)",
                         gst_structure_get_name (structure), features_string);
                g_free (features_string);
            } else {
                g_debug ("%s", gst_structure_get_name (structure));
            }
            gst_structure_foreach (structure, printField, NULL);
        }
    }

private:
    GstElement* e;
    bool debug_output;
};
#endif // GSTELEMENTPROPERTIES_H
