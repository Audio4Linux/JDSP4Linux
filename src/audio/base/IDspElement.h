#ifndef IDSPELEMENT_H
#define IDSPELEMENT_H

#include <DspStatus.h>
#include <DspHost.h>

#include <assert.h>

class DspConfig;

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

    void setMessageHandler(DspHost::MessageHandlerFunc&& extraHandler)
    {
        _msgHandler = std::move(extraHandler);
    }

protected:
    DspHost* _host = nullptr;
    DspHost::MessageHandlerFunc _msgHandler;

};

#endif // IDSPELEMENT_H
