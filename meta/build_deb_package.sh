#!/bin/bash
# Binary needs to be in working directory

deps=""
if [ $2 = "pipewire" ]; then
   deps="qtbase5-dev (>= 5.12.8), libqt5svg5 (>= 5.12.8), libglibmm-2.4-1v5, libglib2.0-0, libpipewire-0.3-0"
elif [ $2 = "pulse" ]; then
   deps="qtbase5-dev (>= 5.12.8), libqt5svg5 (>= 5.12.8), libglibmm-2.4-1v5, libglib2.0-0, libpulse-mainloop-glib0, libgstreamer1.0-0, libgstreamer-plugins-base1.0"
else
  echo "ERROR: Unknown flavor"
  exit 1
fi

debname="jamesdsp-"$2"_"$1"_linux64"
echo $debname
mkdir $debname
mkdir $debname"/DEBIAN"
mkdir -p $debname"/usr/bin"
mkdir -p $debname"/usr/share/applications"
mkdir -p $debname"/usr/share/pixmaps"
cp "jamesdsp" $debname"/usr/bin/jamesdsp"

cp "resources/icons/icon.png" $debname"/usr/share/pixmaps/jamesdsp.png"
cp "LICENSE" $debname"/DEBIAN"

cat <<EOT >> $debname"/usr/share/applications/jamesdsp.desktop"
[Desktop Entry]
Name=JamesDSP
GenericName=Audio effect processor
Comment=JamesDSP for Linux
Keywords=equalizer;audio;effect
Categories=AudioVideo;Audio
Exec=jamesdsp
Icon=/usr/share/pixmaps/jamesdsp.png
StartupNotify=false
Terminal=false
Type=Application
EOT

cat <<EOT >> $debname"/DEBIAN/control"
Package: jamesdsp-$2
Version: $1
Section: sound
Priority: optional
Architecture: amd64
Depends: $deps
Maintainer: Tim Schneeberger (thepbone) <tim.schneeberger@gmail.com>
Description: JamesDSP for Linux
Homepage: https://github.com/Audio4Linux/JDSP4Linux
EOT

dpkg-deb --build $debname
