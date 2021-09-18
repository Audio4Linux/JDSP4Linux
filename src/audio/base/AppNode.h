#ifndef APPNODE_H
#define APPNODE_H

#ifndef USE_PULSEAUDIO
#include "PwPipelineManager.h"
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
        priority = node.priority;
        state = node.state;
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
                state = "error";
                break;
            case PW_NODE_STATE_CREATING:
                state = "creating";
                break;
            case PW_NODE_STATE_SUSPENDED:
                state = "suspended";
                break;
            case PW_NODE_STATE_IDLE:
                state = "idle";
                break;
            case PW_NODE_STATE_RUNNING:
                state = "running";
                break;
        }
    }
#endif

    uint id = SPA_ID_INVALID;

    QString name;

    QString description;

    QString media_class;

    QString app_icon_name;

    QString media_name;

    QString format;

    int priority = -1;

    QString state;

    bool mute = false;

    int n_input_ports = 0;

    int n_output_ports = 0;

    uint rate = 0U;

    int n_volume_channels = 0;

    float latency = 0.0F;

    float volume = 0.0F;

};

Q_DECLARE_METATYPE(AppNode);

#endif // APPNODE_H
