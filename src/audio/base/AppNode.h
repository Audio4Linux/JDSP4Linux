#ifndef APPNODE_H
#define APPNODE_H

#ifndef USE_PULSEAUDIO
#include "PwDataTypes.h"
#else
#include "PulseDataTypes.h"
#endif

#include <QString>
#include <QObject>

#define QSTR(x) QString::fromStdString(x)

class AppNode {

public:
    AppNode(){};

#ifndef USE_PULSEAUDIO
    AppNode(NodeInfo node)
    {
        id = node.id;
        name = QSTR(node.name);
        description = QSTR(node.description);
        media_class = QSTR(node.media_class);
        app_icon_name = QSTR(node.app_icon_name);
        media_name = QSTR(node.media_name);
        format = QSTR(node.format);
        mute = node.mute;
        n_input_ports = node.n_input_ports;
        n_output_ports = node.n_output_ports;
        rate = node.rate;
        n_volume_channels = node.n_volume_channels;
        latency = node.latency;
        volume = node.volume;

        switch(node.state)
        {
            case PW_NODE_STATE_ERROR:
                state = QObject::tr("error");
                break;
            case PW_NODE_STATE_CREATING:
                state = QObject::tr("creating");
                break;
            case PW_NODE_STATE_SUSPENDED:
                state = QObject::tr("suspended");
                break;
            case PW_NODE_STATE_IDLE:
                state = QObject::tr("idle");
                break;
            case PW_NODE_STATE_RUNNING:
                state = QObject::tr("running");
                break;
        }
    }
#else
    AppNode(AppInfo node)
    {
        id = node.index;
        name = QSTR(node.name);
        description = "";
        media_class = QSTR(node.app_type);
        app_icon_name = QSTR(node.icon_name);
        media_name = "";
        format = QSTR(node.format);
        mute = node.mute;
        n_input_ports = node.channels;
        n_output_ports = node.channels;
        rate = node.rate;
        n_volume_channels = node.channels;
        latency = node.latency / 1e+9;
        volume = node.volume;

        if(node.wants_to_play)
            state = QObject::tr("running");
        else if(node.connected)
            state = QObject::tr("idle");
        else
            state = QObject::tr("not connected");
    }
#endif


    uint32_t id = ((uint32_t)0xffffffff);

    QString name;

    QString description;

    QString media_class;

    QString app_icon_name;

    QString media_name;

    QString format;

    QString state;

    bool mute = false;

    int n_input_ports = 0;

    int n_output_ports = 0;

    uint32_t rate = 0U;

    int n_volume_channels = 0;

    float latency = 0.0F;

    float volume = 0.0F;

};

Q_DECLARE_METATYPE(AppNode);

#endif // APPNODE_H
