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
        _msgHandlerReady = true;
    }

    void callMessageHandler(DspHost::Message msg, std::any param)
    {
        if(_msgHandlerReady)
            _msgHandler(msg, param);
        else {
            // TODO fix this case
            // printf("IDspElement::callMessageHandler: dropped message of type '%d'\n", msg);
        }
    }

protected:
    DspHost* _host = nullptr;
    DspHost::MessageHandlerFunc _msgHandler;
    bool _msgHandlerReady = false;

};

#endif // IDSPELEMENT_H
