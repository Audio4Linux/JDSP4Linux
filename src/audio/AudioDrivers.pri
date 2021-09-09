include($$PWD/base/Base.pri)
USE_PULSEAUDIO: include($$PWD/pulseaudio/PulseaudioCore.pri)
else: include($$PWD/pipewire/PipewireCore.pri)

