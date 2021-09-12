#ifndef BASEELEMENT_H
#define BASEELEMENT_H

#include <gst/gst.h>
#include <vector>
#include <string>

class BaseElement
{
public:
    enum class Type{
        SOURCE,
        SINK,
        FILTER
    };

    virtual ~BaseElement() {}

    virtual GstElement*  getGstElement() = 0;
    virtual Type         getBaseType() = 0;
    virtual std::vector
    <GstElement*>        getPartialPipeline() = 0;

    virtual std::string getName()
    {
        return GST_OBJECT_NAME(getGstElement());
    }

    virtual void setValue(const gchar* prop, const GValue value){
        g_object_set_property (G_OBJECT (getGstElement()), prop, &value);
    }

    virtual void setValues(const gchar* firstProp, ...){
        va_list var_args;

        va_start (var_args, firstProp);
        g_object_set_valist (G_OBJECT (getGstElement()), firstProp, var_args);
        va_end (var_args);
    }

    virtual GValue getValue(const gchar* prop){
        GValue value = G_VALUE_INIT;
        g_object_get (G_OBJECT (getGstElement()), prop, &value, NULL);
        return value;
    }

    virtual void getValues(const gchar* firstProp, ...){
        va_list var_args;

        va_start (var_args, firstProp);
        g_object_get_valist (G_OBJECT (getGstElement()), firstProp, var_args);
        va_end (var_args);
    }

};

#endif // BASEELEMENT_H
