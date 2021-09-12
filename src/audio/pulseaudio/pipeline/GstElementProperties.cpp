#include "GstElementProperties.h"


GstElementProperties::GstElementProperties(GstElement* _e,  bool _debug_output)
    : e(_e), debug_output(_debug_output){}

std::list<GstStaticPadTemplate> GstElementProperties::getTemplates()
{
    assert(G_IS_OBJECT(e));

    std::list<GstStaticPadTemplate> templs;

    const GList *pads;
    GstElementFactory* factory = gst_element_get_factory(e);

    if (gst_element_factory_get_num_pad_templates (factory) == 0) {
        return templs;
    }

    pads = gst_element_factory_get_static_pad_templates (factory);
    while (pads) {
        auto templ = *(GstStaticPadTemplate *) (pads->data);
        templs.push_back(templ);
        pads = g_list_next (pads);
    }
    return templs;
}

gboolean GstElementProperties::hasCompatibleCapsWith(GstElement* e2){
    assert(GST_IS_OBJECT(e));
    assert(GST_IS_OBJECT(e2));

    auto e_name = GST_OBJECT_NAME(e);
    auto e2_name = GST_OBJECT_NAME(e2);

    for(GList *l1 = e->pads; l1 != NULL; l1 = l1->next){
        GstPad* src = (GstPad*)l1->data;
        if(!GST_IS_PAD(src) || src->direction != GST_PAD_SRC)
            continue;

        for(GList *l2 = e2->pads; l2 != NULL; l2 = l2->next){
            GstPad* sink = (GstPad*)l2->data;
            if(!GST_IS_PAD(sink) || sink->direction != GST_PAD_SINK)
                continue;

            if(debug_output)
                g_debug("[%s ~> %s] Comparing caps of pad '%s' with pad '%s'",
                        e_name, e2_name, gst_pad_get_name(src), gst_pad_get_name(sink));

            if(haveCompatibleCaps(src, sink)){
                if(debug_output)
                    g_debug("[%s ~> %s] Caps of both pads are compatible", e_name, e2_name);
                return TRUE;
            }
            else
            {
                if(debug_output){
                    g_debug("[%s ~> %s] Caps of both pads have incompatible caps", e_name, e2_name);

                    auto caps_src = gst_pad_template_get_caps(src->padtemplate);
                    auto caps_sink = gst_pad_template_get_caps(sink->padtemplate);

                    g_debug("-------- Caps dump (src) '%s'", e_name);
                    printCaps(caps_src);

                    g_debug("-------- Caps dump (sink) %s", e2_name);
                    printCaps(caps_sink);
                }
            }
        }
    }

    return FALSE;
}

gboolean GstElementProperties::hasTemplate(GstPadDirection direction)
{
    assert(G_IS_OBJECT(e));
    GstElementClass *klass = GST_ELEMENT_GET_CLASS (e);
    GList *l;

    for (l = klass->padtemplates; l != NULL; l = l->next) {
        if (GST_PAD_TEMPLATE (l->data)->direction != direction)
            continue;
        return TRUE;
    }

    return FALSE;
}

gboolean GstElementProperties::hasNonMonoTemplate(GstPadDirection direction)
{
    assert(G_IS_OBJECT(e));
    GstElementClass *klass = GST_ELEMENT_GET_CLASS (e);
    GList *l;

    bool notOnlyMono = false;

    for (l = klass->padtemplates; l != NULL; l = l->next) {
        if (GST_PAD_TEMPLATE (l->data)->direction != direction)
            continue;

        GstCaps* caps = gst_caps_ref (GST_PAD_TEMPLATE_CAPS (l->data));

        for (uint i = 0; i < gst_caps_get_size (caps); i++) {
            GstStructure *structure = gst_caps_get_structure (caps, i);

            int isMono = 2; /* No = 0; Yes = 1; Unrelated key = 2 */
            gst_structure_foreach (structure, [](GQuark field, const GValue* value, void* _is_mono) -> int{
                int *is_mono = static_cast<int*>(_is_mono);
                if(g_strcmp0(g_quark_to_string (field), "channels") == 0){
                    gchar* str = gst_value_serialize(value);
                    if(g_strcmp0(str, "1") == 0){
                        *is_mono = 1;
                        g_free(str);
                        return TRUE;
                    }
                    else{
                        *is_mono = 0;
                        g_free(str);
                        return FALSE;
                    }
                }
                *is_mono = 2;
                return TRUE;
            }, &isMono);

            /* At least one pad is not limited to mono output */
            if(isMono == 0){
                notOnlyMono = true;
                break;
            }

        }

        gst_caps_unref(caps);
    }

    return notOnlyMono;
}

gboolean GstElementProperties::hasSometimesTemplate(GstPadDirection direction)
{
    assert(G_IS_OBJECT(e));
    GstElementClass *klass = GST_ELEMENT_GET_CLASS (e);
    GList *l;

    for (l = klass->padtemplates; l != NULL; l = l->next) {
        if (GST_PAD_TEMPLATE (l->data)->direction != direction)
            continue;
        if (GST_PAD_TEMPLATE (l->data)->presence == GST_PAD_SOMETIMES)
            return TRUE;
    }

    return FALSE;
}

gboolean GstElementProperties::isSometimesPadRequired(GstPadDirection direction)
{
    return !hasAlwaysTemplate(direction) && hasSometimesTemplate(direction);
}

gboolean GstElementProperties::hasAlwaysTemplate(GstPadDirection direction)
{
    assert(G_IS_OBJECT(e));
    GstElementClass *klass = GST_ELEMENT_GET_CLASS (e);
    GList *l;

    for (l = klass->padtemplates; l != NULL; l = l->next) {
        if (GST_PAD_TEMPLATE (l->data)->direction != direction)
            continue;
        if (GST_PAD_TEMPLATE (l->data)->presence == GST_PAD_ALWAYS)
            return TRUE;
    }

    return FALSE;
}

gboolean GstElementProperties::hasOnlyUnsupportedPads(GstPadDirection direction)
{
    return hasTemplate(direction) && !hasAlwaysTemplate(direction) && !hasSometimesTemplate(direction);
}

GstPadPresence *GstElementProperties::isLinkablePadAvailable(GstPadDirection direction)
{
    assert(G_IS_OBJECT(e));
    GstElementClass *klass = GST_ELEMENT_GET_CLASS (e);
    GList *l;

    for (l = klass->padtemplates; l != NULL; l = l->next) {
        if (GST_PAD_TEMPLATE (l->data)->direction != direction)
            continue;
        if (GST_PAD_TEMPLATE (l->data)->presence == GST_PAD_ALWAYS)
            return &GST_PAD_TEMPLATE (l->data)->presence;
    }

    for (l = klass->padtemplates; l != NULL; l = l->next) {
        if (GST_PAD_TEMPLATE (l->data)->direction != direction)
            continue;
        if (GST_PAD_TEMPLATE (l->data)->presence == GST_PAD_SOMETIMES)
            return &GST_PAD_TEMPLATE (l->data)->presence;
    }

    return NULL;
}

void GstElementProperties::printPadTemplates()
{
    const GList *pads;
    GstStaticPadTemplate *padtemplate;
    GstPadTemplate *tmpl;

    GstElementFactory* factory = gst_element_get_factory(e);

    g_debug ("Pad Templates:\n");

    if (gst_element_factory_get_num_pad_templates (factory) == 0) {
        g_debug ("\tnone\n");
        return;
    }

    pads = gst_element_factory_get_static_pad_templates (factory);
    while (pads) {
        padtemplate = (GstStaticPadTemplate *) (pads->data);
        pads = g_list_next (pads);

        if (padtemplate->direction == GST_PAD_SRC)
            g_debug ("\tSRC template: '%s'\n", padtemplate->name_template);
        else if (padtemplate->direction == GST_PAD_SINK)
            g_debug ("\tSINK template: '%s'\n", padtemplate->name_template);
        else
            g_debug ("\tUNKNOWN template: '%s'\n", padtemplate->name_template);

        if (padtemplate->presence == GST_PAD_ALWAYS)
            g_debug ("\tAvailability: Always\n");
        else if (padtemplate->presence == GST_PAD_SOMETIMES)
            g_debug ("\tAvailability: Sometimes\n");
        else if (padtemplate->presence == GST_PAD_REQUEST) {
            g_debug ("\tAvailability: On request\n");
        } else
            g_debug ("\tAvailability: UNKNOWN\n");

        if (padtemplate->static_caps.string) {
            GstCaps *caps = gst_static_caps_get (&padtemplate->static_caps);

            g_debug ("Capabilities:\n");
            printCaps (caps);
            gst_caps_unref (caps);
        }

        tmpl = gst_element_class_get_pad_template (GST_ELEMENT_GET_CLASS (e),
                                                   padtemplate->name_template);
        if (tmpl != NULL) {
            GType pad_type = GST_PAD_TEMPLATE_GTYPE (tmpl);

            if (pad_type != G_TYPE_NONE && pad_type != GST_TYPE_PAD) {
                gpointer pad_klass;

                pad_klass = g_type_class_ref (pad_type);
                g_debug ("Type: %s\n", g_type_name (pad_type));
                g_type_class_unref (pad_klass);
            }
        }

    }
}

gboolean GstElementProperties::hasSpecificTemplate(GstPadPresence pres, GstPadDirection direction)
{
    assert(G_IS_OBJECT(e));
    GstElementClass *klass = GST_ELEMENT_GET_CLASS (e);
    GList *l;

    for (l = klass->padtemplates; l != NULL; l = l->next) {
        if (GST_PAD_TEMPLATE (l->data)->direction != direction)
            continue;
        if (GST_PAD_TEMPLATE (l->data)->presence == pres)
            return TRUE;
    }

    return FALSE;
}

void GstElementProperties::printPadsOfElement(GstElement *dest){
    g_debug("------> [%s]", GST_OBJECT_NAME(e));

    if(dest)
        g_debug("%s -> %s", GST_OBJECT_NAME(e), GST_OBJECT_NAME(dest));

    if(!hasTemplate(GST_PAD_SRC))
        g_debug("[%s] No SRC pad found", GST_OBJECT_NAME(e));
    else{
        g_debug("[%s] SRC pad is always linkable: %s", GST_OBJECT_NAME(e),
                hasAlwaysTemplate(GstPadDirection::GST_PAD_SRC) ? "yes" : "no");
    }
    if(!hasTemplate(GST_PAD_SINK))
        g_debug("[%s] No SINK pad found", GST_OBJECT_NAME(e));
    else{
        g_debug("[%s] SINK pad is always linkable: %s", GST_OBJECT_NAME(e),
                hasAlwaysTemplate(GstPadDirection::GST_PAD_SINK) ? "yes" : "no");
    }
}
