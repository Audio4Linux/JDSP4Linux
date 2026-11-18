## Build from sources (Pipewire)

This document only contains compile instructions for the PipeWire build. Please read [INSTALL_PULSE.md](INSTALL_PULSE.md) if you want to compile the legacy PulseAudio version instead.

### Install dependencies

**Debian/Ubuntu-based distros**

```bash
sudo apt install build-essential qt6-qpa-plugins libarchive-dev qt6-base-private-dev qtbase5-private-dev qt6-base-dev libqt6svg6-dev libglibmm-2.4-dev libglib2.0-dev libpipewire-0.3-dev qttools5-dev-tools libgl-dev
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

> [!IMPORTANT]  
>
> On some distros like Ubuntu, `qmake6` instead of `qmake` should be used.

Execute compiled binary

```bash
./src/jamesdsp
```

### Live DSP development

Start the source build with `--watch-liveprog` to reload the selected, enabled
LiveProg `.eel` script when an external editor saves it:

```bash
./src/jamesdsp --watch-liveprog
```

Select a script and enable LiveProg in the GUI. The watcher also follows script
changes made through D-Bus or the CLI. `examples/live-development.eel` provides
a unity-gain starting point. It waits for 150 ms without a file event
before reloading. Both direct writes and atomic file replacements are supported.
An unchanged file or an unrelated file in the same directory does not reload the
program. A missing, unreadable, or empty script leaves the current program loaded;
restore the file in the same directory to resume. Keep that directory in place.
Disable LiveProg to stop watching it. Without this option, save behavior is unchanged.

Reload uses the existing LiveProg compiler and resets script state. Compilation
results are written to the log. A syntax error follows the existing compiler's
behavior; this mode does not promise to retain the previous program after a syntax
error. It also does not bound the execution time or output level of authored DSP
code. The master switch, gain, EQ, and output device are not changed by the watcher.
Enable the master switch when you want to hear the selected effects.

For native C/C++ changes, use the build loop from the repository root. It needs
Python 3.9 or later and `inotifywait` (`sudo apt install inotify-tools` on Ubuntu):

```bash
python3 tools/dev.py -- --tray
```

The loop builds the application and starts it with `--watch-liveprog`. Close an
existing JamesDSP process first; the loop refuses to take over another process.
Native source, header, qmake, UI, and selected resource file events cause a rebuild.
EEL files use the in-process watcher and do not cause native rebuilds. Build and
Git directories are excluded. `--build-dir PATH`, `--qmake PATH`, and `--jobs N`
select the build settings. Use an ordinary GUI PipeWire build directory.

A failed build keeps the current process running. After a successful build, the
loop copies the binary and restarts only its own child. This restart can briefly
interrupt sound; native code is not replaced inside the running process. Settings
remain in the normal JamesDSP configuration directory. The build directory holds
`build.log`, `application.log`, and the running binary at `run/jamesdsp`. An
unexpected application exit stops the loop and reports the log path. Ctrl-C stops
the loop and its child.

Run the file watcher and process control tests from the repository root:

```bash
mkdir -p build/tests/liveprog-watcher
cd build/tests/liveprog-watcher
qmake6 ../../../tests/liveprog-watcher/liveprog-watcher.pro
make -j4
./tst_liveprogwatcher
cd ../../..
python3 -m unittest discover -s tests -p test_dev.py -v
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
