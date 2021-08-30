#ifndef FILESOURCEELEMENT_H
#define FILESOURCEELEMENT_H

#include <string>
#include <vector>
#include <gst/gst.h>

#include "SourceElement.h"

class FileSourceElement : public SourceElement
{
public:
    FileSourceElement(std::string _path);
    ~FileSourceElement();

    std::vector
    <GstElement*>   getPartialPipeline();
    GstElement*     getGstElement();
    SrcType         getType();
    std::string     getPath() const;
    void            setPath(const std::string &value);

private:
    std::string     path;
    GstElement      *source, *decoder;
};

#endif // FILESOURCEELEMENT_H
