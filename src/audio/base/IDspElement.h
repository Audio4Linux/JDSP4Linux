#ifndef IDSPELEMENT_H
#define IDSPELEMENT_H

#include <DspStatus.h>
#include <assert.h>

class DspConfig;
class DspHost;

class IDspElement
{
public:
    virtual ~IDspElement(){};

    virtual DspStatus status() = 0;

    DspHost* host()
    {
       assert(_host != nullptr);
       return _host;
    }

protected:
    DspHost* _host = nullptr;

};

#endif // IDSPELEMENT_H
