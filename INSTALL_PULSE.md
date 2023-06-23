## Installation for PulseAudio

This section is dedicated to systems using PulseAudio as the audio server. If you are using PipeWire, please go [here](https://github.com/Audio4Linux/JDSP4Linux#installation).

* [Flatpak](#flatpak)
* [Arch Linux (AUR)](#arch)
* [Build from sources](#build-from-sources)

### Flatpak

Universal binary packages for all distros.

The legacy PulseAudio build is not available on FlatHub.. You need to retrieve it from my personal repository:
```
sudo flatpak remote-add --if-not-exists thepbones-repo https://raw.githubusercontent.com/ThePBone/flatpak-repo/main/thepbone.flatpakrepo
flatpak install me.timschneeberger.jdsp4linux.pulse
```

> **Note**: Flatpaks are sandboxed. This application can only access `~/.var/app/me.timschneeberger.jdsp4linux.pulse/` by default.

### Arch
[AUR packages](https://aur.archlinux.org/packages/?O=0&K=jamesdsp) are available:

* Stable version

   ![AUR version](https://img.shields.io/aur/version/jamesdsp-pulse) ![AUR version](https://img.shields.io/aur/votes/jamesdsp-pulse) ![AUR version](https://img.shields.io/aur/maintainer/jamesdsp-pulse) ![AUR version](https://img.shields.io/aur/last-modified/jamesdsp-pulse)
   ```
   yay -S jamesdsp-pulse
   ```

* Development version

   ![AUR version](https://img.shields.io/aur/version/jamesdsp-pulse-git) ![AUR version](https://img.shields.io/aur/votes/jamesdsp-pulse-git) ![AUR version](https://img.shields.io/aur/maintainer/jamesdsp-pulse-git) ![AUR version](https://img.shields.io/aur/last-modified/jamesdsp-pulse-git)
   ```
   yay -S jamesdsp-pulse-git
   ```

### Build from sources

#### Install dependencies

**Debian/Ubuntu-based distros**

```bash
sudo apt install build-essential libarchive-dev qtbase5-private-dev qtbase5-dev libqt5svg5-dev libglibmm-2.4-dev libglib2.0-dev libpulse-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
```
**Fedora**

```bash
sudo dnf install libarchive-devel qt5-qtbase-devel qt5-qtbase-private-devel qt5-qtsvg-devel glibmm24-devel glib2-devel pulseaudio-libs-devel gstreamer1-devel gstreamer1-plugins-base-devel 
```
**Arch Linux**

```
sudo pacman -S gcc make pkgconfig libarchive qt5-base qt5-svg glib2 glibmm libpulse gst-plugins-good gstreamer 
```

#### Build application

Clone git repositories and submodules:

```bash
git clone --recursive https://github.com/Audio4Linux/JDSP4Linux
```

Prepare build environment

```bash
cd JDSP4Linux
mkdir build
cd build
# Compile app
qmake ../JDSP4Linux.pro "CONFIG += USE_PULSEAUDIO"
make -j4
```

Execute compiled binary

```bash
./src/jamesdsp
```

#### Optional: Manual installation + menu entry

Copy binary to /usr/local/bin and set permissions

```bash
sudo cp src/jamesdsp /usr/local/bin
sudo chmod 755 /usr/local/bin/jamesdsp
```

Create a menu entry

```bash
sudo sh -c 'sudo cat <<EOT >> /usr/share/applications/jamesdsp.desktop
[Desktop Entry]
Name=JamesDSP
GenericName=Audio effect processor
Comment=JamesDSP for Linux  (Legacy PulseAudio version)
Keywords=equalizer;audio;effect
Categories=AudioVideo;Audio;
Exec=jamesdsp
Icon=/usr/share/pixmaps/jamesdsp.png
StartupNotify=false
Terminal=false
Type=Application
EOT'
```

Download icon

```bash
sudo wget -O /usr/share/pixmaps/jamesdsp.png https://raw.githubusercontent.com/Audio4Linux/JDSP4Linux/master/resources/icons/icon.png -q --show-progress
```