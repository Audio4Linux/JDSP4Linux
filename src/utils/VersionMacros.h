#ifndef VERSIONMACROS_H
#define VERSIONMACROS_H

#define STR_(x) #x
#define STR(x) STR_(x)

#ifdef USE_PULSEAUDIO
#define APP_FLAVOR "(Pulseaudio flavor)"
#define APP_FLAVOR_SHORT "Pulseaudio"
#else
#define APP_FLAVOR "(Pipewire flavor)"
#define APP_FLAVOR_SHORT "Pipewire"
#endif

#define APP_VERSION_FULL STR(APP_VERSION) " " APP_FLAVOR

#endif // VERSIONMACROS_H
