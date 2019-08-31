# JDSP4Linux-GUI

User Interface for JDSP4Linux [https://github.com/ThePBone/JDSP4Linux](https://github.com/ThePBone/JDSP4Linux)
* Telegram: @ThePBone

This project is work-in-progress.

## Installation
* [Portable/Manually](#portablemanually)

### Portable/Manually
#### Install Dependencies
You will need to change this line, depending on which package manager your distribution uses.
```bash
sudo apt install qt5-qmake libqt5widgets5 libqt5gui5 libqt5core5a libqt5multimedia5 libqt5xml5 libgl1-mesa-dev git
```

#### Build from sources
Clone this repository

    git clone https://github.com/ThePBone/JDSP4Linux-GUI

Compile sources

    cd JDSP4Linux-GUI
    qmake
    make
    
```bash
./jdsp-gui
```

#### Optional: Manual Install
##### Copy to /usr/local/bin
```bash
sudo cp jdsp-gui /usr/local/bin
sudo chmod 755 /usr/local/bin/jdsp-gui
```
##### Create Menu Entry
```bash
sudo cat <<EOT >> /usr/share/applications/jdsp-gui.desktop
[Desktop Entry]
Name=JDSP4Linux
GenericName=Equalizer
Comment=User Interface for JDSP4Linux
Keywords=equalizer
Categories=AudioVideo;Audio;
Exec=jdsp-gui
Icon=/usr/share/pixmaps/jdsp-gui.png
StartupNotify=false
Terminal=false
Type=Application
EOT
```
##### Download Icon
```bash
sudo wget -O /usr/share/pixmaps/jdsp-gui.png https://raw.githubusercontent.com/ThePBone/JDSP4Linux-GUI/master/icons/icon.png -q --show-progress
```
## Credits
* [Material Icons](https://material.io/tools/icons/)
* [QSS Stylesheets - GTRONICK (modified by ThePBone)](https://github.com/GTRONICK/QSS)