## Build from sources (Pipewire)

This document only contains compile instructions for the PipeWire build. Please read [INSTALL_PULSE.md](INSTALL_PULSE.md) if you want to compile the legacy PulseAudio version instead.

### Install dependencies

**Debian/Ubuntu-based distros**

```bash
sudo apt install build-essential libarchive-dev qt6-base-private-dev qt6-base-dev libqt6svg6-dev libglibmm-2.4-dev libglib2.0-dev libpipewire-0.3-dev qttools5-dev-tools
```

**Fedora 34**

```bash
sudo dnf install libarchive-devel qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtsvg-devel glibmm24-devel glib2-devel pipewire-devel
```

**Arch Linux**

```bash
sudo pacman -S gcc make pkgconfig libarchive qt6-base qt6-svg glib2 glibmm pipewire
```

### Build application

Clone git repositories and submodules:

```bash
git clone --recursive https://github.com/Audio4Linux/JDSP4Linux
```

Prepare build environment

```bash
cd JDSP4Linux
mkdir build
cd build
```

Compile the app

> [!NOTE]  
> By specifing the `HEADLESS` flag, you can build JamesDSP as a pure CLI app without the GUI.
> `qmake ../JDSP4Linux.pro CONFIG+=HEADLESS`

```bash
qmake ../JDSP4Linux.pro
make -j4
```

Execute compiled binary

```bash
./src/jamesdsp
```

### Optional: Manual installation + menu entry

Copy the binary to /usr/local/bin and set permissions

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
Comment=JamesDSP for Linux
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
