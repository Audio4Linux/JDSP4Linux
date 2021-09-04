#include "FileSourceElement.h"

FileSourceElement::FileSourceElement(std::string _path)
{
    path = _path;

    source  = gst_element_factory_make ("filesrc",      "filesrc");
    decoder = gst_element_factory_make ("decodebin",    "decodebin");

    /* set the input filename to the source element */
    g_object_set (G_OBJECT (source), "location", path.c_str(), NULL);

    gst_object_ref(source);
    gst_object_ref(decoder);
}

FileSourceElement::~FileSourceElement()
{
    gst_object_unref(source);
    gst_object_unref(decoder);
}

std::string FileSourceElement::getPath() const
{
    return path;
}

void FileSourceElement::setPath(const std::string &value)
{
    path = value;
}

std::vector<GstElement*> FileSourceElement::getPartialPipeline(){

    std::vector<GstElement*> pipe;
    pipe.push_back(source);
    pipe.push_back(decoder);
    return pipe;
}


GstElement* FileSourceElement::getGstElement()
{
    return source;
}

SourceElement::SrcType FileSourceElement::getType()
{
    return SrcType::File;
}


