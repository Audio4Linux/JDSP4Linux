#!/bin/bash
echo "Validating metainfo.xml files"
flatpak run org.freedesktop.appstream-glib validate me.timschneeberger.jdsp4linux.metainfo.xml
flatpak run org.freedesktop.appstream-glib validate me.timschneeberger.jdsp4linux.pulse.metainfo.xml
echo "Validating desktop files"
desktop-file-validate me.timschneeberger.jdsp4linux.desktop
desktop-file-validate me.timschneeberger.jdsp4linux.pulse.desktop
