# JDSP4Linux-GUI

User Interface for JDSP4Linux [https://github.com/ThePBone/JDSP4Linux](https://github.com/ThePBone/JDSP4Linux)
* Telegram: @ThePBone
* [Releases](https://github.com/ThePBone/JDSP4Linux-GUI/releases)

This project is work-in-progress.

## Installation
* [Arch](#arch)
* [Ubuntu PPA](#ubuntu-ppa)
* [Portable/Manually](#portablemanually)

### Arch
On Arch you can install the [AUR package](https://aur.archlinux.org/packages/jdsp4linux-gui-git/) by yochananmarqos:
```bash
yay -S jdsp4linux-gui-git
```

Or [this one](https://aur.archlinux.org/packages/jdsp4linux-gui) to get the latest stable release:
```bash
yay -S jdsp4linux-gui
```
![AUR version](https://img.shields.io/aur/version/jdsp4linux-gui?label=aur%20%28stable%29) ![AUR version](https://img.shields.io/aur/version/jdsp4linux-gui-git?label=aur%20%28git%29)

### Ubuntu (PPA)
#### Add PPA Repo
```bash
curl -s --compressed "https://thepbone.github.io/PPA-Repository/KEY.gpg" | sudo apt-key add -
sudo curl -s --compressed -o /etc/apt/sources.list.d/thepbone_ppa.list "https://thepbone.github.io/PPA-Repository/thepbone_ppa.list"
sudo apt update
```
#### Install from PPA
```bash
sudo apt install jdsp4linux-gui
```
[View PPA on GitHub](https://github.com/ThePBone/PPA-Repository)

### Portable/Manually
#### Install Dependencies
You will need to change this line, depending on which package manager your distribution uses.
```bash
sudo apt install qt5-qmake libqt5widgets5 libqt5gui5 libqt5core5a libqt5multimedia5 libqt5xml5 libgl1-mesa-dev git
```

#### Build from sources
_(You can find precompiled binaries [here](https://github.com/ThePBone/JDSP4Linux-GUI/releases))_

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
## Screenshots
![Screenshot](https://github.com/ThePBone/JDSP4Linux-GUI/blob/master/screenshots/screenshot_reverb.png?raw=true)

## Contributors
* [yochananmarqos - AUR packages](https://github.com/yochananmarqos)
##### Other credits
* [GTRONICK - QSS Stylesheets (modified by ThePBone)](https://github.com/GTRONICK/QSS)
* [Material Icons](https://material.io/tools/icons/)
