<h1 align="center">
  <img alt="Icon" width="75" src="https://github.com/Audio4Linux/JDSP4Linux/blob/master/resources/icons/icon.png?raw=true">
  <br>
  JamesDSP for Linux
  <br>
</h1>
<h4 align="center">Open-source sound effects for PipeWire and PulseAudio</h4>
<p align="center">
  <a href="https://github.com/Audio4Linux/JDSP4Linux/releases">
  	<img alt="GitHub release (latest by date)" src="https://img.shields.io/github/v/release/Audio4Linux/JDSP4Linux">
  </a>
  <a href="https://github.com/Audio4Linux/JDSP4Linux/blob/master/LICENSE">
      <img alt="License" src="https://img.shields.io/github/license/Audio4Linux/JDSP4Linux">
  </a>
  <a href="https://github.com/Audio4Linux/JDSP4Linux/">
    <img alt="Windows build" src="https://img.shields.io/github/repo-size/Audio4Linux/JDSP4Linux">
  </a>
</p>
<p align="center">
  <a href="#features">Features</a> •
  <a href="#installation">Installation</a> •
  <a href="#scripting--ipc-apis">Scripting/API</a> •
  <a href="#contributors">Contributors</a> •
  <a href="#license">License</a> 
</p>

<p align="center">
   <img alt="Screenshot" width="702" src="https://github.com/Audio4Linux/JDSP4Linux/blob/master/meta/screenshot.png?raw=true">
</p>

<p align="center">
Linux port developed by <a href="https://github.com/thepbone">Tim Schneeberger (@thepbone)</a>
<p/><p align="center">
<a href="https://github.com/james34602/JamesDSPManager">JamesDSP</a> was initially published as an audio effects processor<br>for Android devices and is written by <a href="https://github.com/james34602">James Fung (@james34602)</a>.
</p>
<p align="center">
    Feel free to join our <a href="https://t.me/joinchat/FTKC2A2bolHkFAyO-fuPjw">Telegram group</a> for support and updates
</p>

____________

<p align="center">
    <a href="https://crowdin.com/project/jdsp4linux" rel="nofollow"><img style="width:140;height:40px" src="https://badges.crowdin.net/badge/light/crowdin-on-dark.png" srcset="https://badges.crowdin.net/badge/light/crowdin-on-dark.png 1x,https://badges.crowdin.net/badge/light/crowdin-on-dark@2x.png 2x" alt="Crowdin" /></a>
    <br/>
    Please help us to <a href="https://crowdin.com/project/jdsp4linux">translate this app</a> on <a href="https://crowdin.com/project/jdsp4linux">Crowdin</a>!
</p>

## Features

* Automatic bass boost
  * Frequency-detecting bass-boost. Automatically sets its own parameters, such as gain, bandwidth, and cut-off frequency, by analyzing the incoming audio stream
* Automatic dynamic range compressor
  * A highly automated multiband dynamic range adjusting effect
* Complex reverberation IIR network (Progenitor 2)
* Interpolated FIR equalizer with flexible bands
* Arbitrary response equalizer (also known as GraphicEQ from EqualizerAPO)
  * AutoEQ database integration (requires network connection)
* Partitioned convolver (Auto segmenting convolution)
  * Supports mono, stereo, full/true stereo (LL, LR, RL, RR) impulse response
* Crossfeed
  * Realistic surround effects
* Soundstage wideness
  * A multiband stereo wideness controller
* ViPER-DDC
  * Perform parametric equalization on audio
  * Create VDC input files using [thepbone/DDCToolbox](https://github.com/thepbone/DDCToolbox)
* Analog modeling
  * An aliasing-free even harmonic generator
* Output limiter
* **Scripting engine: Live programmable DSP**
  * Write your own audio effects using the [EEL2 scripting language](https://github.com/james34602/EEL_CLI)
  * Auto-generate a basic user interface for your script to modify specific parameters/constants without editing the actual code
  * The scripting language has been extended using many DSP-related functions for easy access, for example, spectral processing, constant Q transform, multi-purpose FIR filter designer IIR sub-bands transformation, etc...
  * **This app also includes a custom minimal scripting IDE**:
    * Syntax highlighting
    * Basic code completion
    * Dynamic code outline window
    * Console output support
    * Detailed error messages with inline code highlighting


## PipeWire vs PulseAudio

**Designed for use with PipeWire. PulseAudio is only supported for backward compatibility.**

PipeWire has a much lower latency compared to PulseAudio when injecting audio effects processors into the audio pipeline. 
I'm currently not planning to add more advanced support for Pulseaudio clients. Features such as selective app exclusion, changing the target audio device, and similar features will only be available to PipeWire clients.

### Which one am I using?

Follow the instructions below if you don't know which one your Linux distribution is using. If you already know, skip to the 'Install dependencies' section.

Run `LC_ALL=C pactl info | grep "Server Name:"` in your terminal. 

If you are using **Pipewire** the output should look similar to this:
```
Server Name: PulseAudio (on PipeWire 0.3.35)
```

If you are using **Pulseaudio** the output should look exactly like this:
```
Server Name: pulseaudio
```

## Installation

**Decide whether you need to install the PipeWire or PulseAudio version of this app!**

If you don't know which version fits your Linux setup, go to the [PipeWire vs PulseAudio section](#which-one-am-i-using) above.

It is recommended to switch to PipeWire, if possible. JamesDSP's audio backend for PulseAudio is in maintance-mode; however, it will continue to receive UI-related feature updates.

The installation instructions for the PulseAudio version have been moved to a separate file: [INSTALL_PULSE.md](INSTALL_PULSE.md).

## Installation for PipeWire
This section is dedicated to systems using PipeWire as the audio server. If you are still using PulseAudio, please go [here](INSTALL_PULSE.md).

* [Flatpak](#flatpak)
* [Arch Linux (AUR)](#arch)
* [Fedora/openSUSE](#fedoraopensuse)
* [Build from sources](#build-from-sources)

### Flatpak

Universal binary packages for all distros.

The recommended **Pipewire version** is available for download on FlatHub: https://flathub.org/apps/me.timschneeberger.jdsp4linux
```
flatpak install me.timschneeberger.jdsp4linux
```

<a href='https://flathub.org/apps/me.timschneeberger.jdsp4linux'><img width='240' alt='Download on Flathub' src='https://dl.flathub.org/assets/badges/flathub-badge-en.png'/></a>


If you are still using **PulseAudio**, you need to download the legacy package from my personal repository:
```
sudo flatpak remote-add --if-not-exists thepbones-repo https://raw.githubusercontent.com/ThePBone/flatpak-repo/main/thepbone.flatpakrepo
flatpak install me.timschneeberger.jdsp4linux.pulse
```

> **Note**: Flatpaks are sandboxed. This application can only access `~/.var/app/me.timschneeberger.jdsp4linux/` by default.

### Arch
[AUR packages](https://aur.archlinux.org/packages/?O=0&K=jamesdsp) are available:

* Stable version

   ![AUR version](https://img.shields.io/aur/version/jamesdsp) ![AUR version](https://img.shields.io/aur/votes/jamesdsp) ![AUR version](https://img.shields.io/aur/maintainer/jamesdsp) ![AUR version](https://img.shields.io/aur/last-modified/jamesdsp)
   ```
   yay -S jamesdsp
   ```

* Development version

   ![AUR version](https://img.shields.io/aur/version/jamesdsp-git) ![AUR version](https://img.shields.io/aur/votes/jamesdsp-git) ![AUR version](https://img.shields.io/aur/maintainer/jamesdsp-git) ![AUR version](https://img.shields.io/aur/last-modified/jamesdsp-git)
   ```
   yay -S jamesdsp-git
   ```

### Fedora/openSUSE

Package maintained by [@theAeon](https://github.com/theAeon) on [Fedora COPR](https://copr.fedorainfracloud.org/coprs/arrobbins/JDSP4Linux/).
Built for Fedora 34/35/Rawhide and OpenSUSE Tumbleweed.

```
yum copr enable arrobbins/JDSP4Linux && yum update && yum install jamesdsp
```

If you are still using PulseAudio with your Fedora/openSUSE installation, refer to the '[Build from sources](#build-from-sources)' section below instead.

### Build from sources

#### Install dependencies

**Debian/Ubuntu-based distros**

```bash
sudo apt install build-essential libarchive-dev qtbase5-private-dev qtbase5-dev libqt5svg5-dev libglibmm-2.4-dev libglib2.0-dev libpipewire-0.3-dev 
```

**Fedora 34**

```bash
sudo dnf install libarchive-devel qt5-qtbase-devel qt5-qtbase-private-devel qt5-qtsvg-devel glibmm24-devel glib2-devel pipewire-devel
```

**Arch Linux**

```bash
sudo pacman -S gcc make pkgconfig libarchive qt5-base qt5-svg glib2 glibmm pipewire
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
qmake ../JDSP4Linux.pro
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
sudo sh -c 'sudo cat <<EOT >> /usr/share/applications/jamesdsp-test.desktop
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
## Scripting & IPC APIs

Since version 2.5.0, this app supports IPC via D-Bus and is also configurable via a CLI.

### Remote control via CLI
You can list all supported commands using `jamesdsp --help`. 
Currently, these commands for remote-controlling JamesDSP's audio engine are available:
```
  --is-connected               Check if JamesDSP service is active. Returns exit code 1 if not. (Remote)
  --list-keys                  List available audio configuration keys (Remote)
  --get <key>                  Get audio configuration value (Remote)
  --set <key=value>            Set audio configuration value (format: key=value) (Remote)
  --load-preset <name>         Load preset by name (Remote)
  --save-preset <name>         Save current settings as preset (Remote)
  --delete-preset <name>       Delete preset by name (Remote)
  --list-presets               List presets (Remote)
  --status                     Show status (Remote)
```
The options should be fairly self-explanatory. For example, `jamesdsp --set reverb_enable=true` would enable the reverberation setting. Have a look at the audio configuration file at `~/.config/jamesdsp/audio.conf` to learn more about possible setting keys and their syntax.

> **Note**: These commands try to connect to an active JamesDSP instance. If no instance is currently online, they will fall-back to modifying the audio configuration file directly on disk. The `--is-connected` option can be used to check whether one is currently online.

### D-Bus IPC

This app also exposes a D-Bus service on the session bus which can be used by other developers or users:

Service name: `me.timschneeberger.jdsp4linux`
* GUI-related interface:
  * Path name: `/jdsp4linux/gui`
  * Interface name: `me.timschneeberger.jdsp4linux.Gui`
* Audio service-related interface:
  * Path name: `/jdsp4linux/service`
  * Interface name: `me.timschneeberger.jdsp4linux.Service`

If you want to test it out, you can use an app like [D-Feet](https://wiki.gnome.org/Apps/DFeet) to interact with the D-Bus services.

The D-Bus introspection XML is available here: https://github.com/Audio4Linux/JDSP4Linux/blob/master/src/utils/dbus/manifest.xml.

## Troubleshooting
* My volume control not working anymore
  * Don't set the virtual JamesDSP device as the default audio output device. The virtual device has no audio volume controls and should never be used directly. Please set your actual speakers/headphones as the default output device instead.

* Crackling audio
  * Your CPU may be too slow to process the audio sample in time; try to disable some effects (especially resource-hungry ones like the convolver)
  * Set JamesDSP's process to real-time or high priority using a task manager of your choice
  * [Pipewire] Try out the workaround mentioned in [issue #47](https://github.com/Audio4Linux/JDSP4Linux/issues/47)

## Screenshots

<p align="center">
   <img alt="Screenshot" width="702" src="https://github.com/Audio4Linux/JDSP4Linux/blob/master/meta/screenshot_presets.png?raw=true">  
   <img alt="Screenshot" width="702" src="https://github.com/Audio4Linux/JDSP4Linux/blob/master/meta/screenshot_eel.png?raw=true"> 
   <img alt="Screenshot" width="702" src="https://github.com/Audio4Linux/JDSP4Linux/blob/master/meta/screenshot_graphiceq.png?raw=true"> 
   <img alt="Screenshot" width="702" src="https://github.com/Audio4Linux/JDSP4Linux/blob/master/meta/screenshot_ide.png?raw=true">
</p>

## Contributors

* [James Fung](https://github.com/james34602) - Developer of the core library ['libjamesdsp'](https://github.com/james34602/JamesDSPManager/tree/master/Main)
* [yochananmarqos](https://github.com/yochananmarqos) - AUR packages
* [theAeon](https://github.com/theAeon) - RPM packages
* PipeWire/Pulse implementation based on [EasyEffects](https://github.com/wwmm/EasyEffects) by [Wellington Wallace](https://github.com/wwmm)

### Translators

<!-- CROWDIN-CONTRIBUTORS-START -->
<table>
  <tr>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/ThePBone"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15683553/medium/d13428d1e0922bc2069500aef57d1459.png" />
        <br />
        <sub><b>Tim Schneeberger (ThePBone)</b></sub></a>
      <br />
      <sub><b>7284 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/Kazevic"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15680393/medium/4393ae8969da30fc9475409e95e74867.png" />
        <br />
        <sub><b>Kazevic</b></sub></a>
      <br />
      <sub><b>3173 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/Camellan"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/13410766/medium/b4019516b3323e817b7e77712961de69_default.png" />
        <br />
        <sub><b>Camellan</b></sub></a>
      <br />
      <sub><b>3152 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/KatieFrogs"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/14450708/medium/bb568293d8b26daa3f7f323dbc9d69d0.png" />
        <br />
        <sub><b>Katie Frogs (KatieFrogs)</b></sub></a>
      <br />
      <sub><b>2848 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/catvinyl"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15923209/medium/87c1ec8379dee277ba0bd42e1d8206f2.png" />
        <br />
        <sub><b>catvinyl</b></sub></a>
      <br />
      <sub><b>2352 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/mefsaal"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/13221907/medium/c0b751a37f076028f7271b00392736aa.jpg" />
        <br />
        <sub><b>Gabriel Cabrera Davila (mefsaal)</b></sub></a>
      <br />
      <sub><b>652 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/seqfault"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15878639/medium/bf4af8eeefcd4c065fd867a7ad16994b.jpeg" />
        <br />
        <sub><b>NullPointerException (seqfault)</b></sub></a>
      <br />
      <sub><b>1151 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/mariachini"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/13113640/medium/99ff648dd8f28efebdce9713cee1b9c3.png" />
        <br />
        <sub><b>mariachini</b></sub></a>
      <br />
      <sub><b>373 words</b></sub>
    </td>
  </tr>
  <tr>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/zhanghua000"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15906451/medium/db06f53e9b83801ec99389ab759b1e48.png" />
        <br />
        <sub><b>Noob Zhang (zhanghua000)</b></sub></a>
      <br />
      <sub><b>240 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/AnClark"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15932859/medium/621b49aa22122e6ca25fc819eac5e96f.jpeg" />
        <br />
        <sub><b>AnClark Liu (AnClark)</b></sub></a>
      <br />
      <sub><b>173 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/so1ar"><img alt="logo" style="width: 64px" src="https://i2.wp.com/crowdin.com/images/user-picture.png?ssl=1" />
        <br />
        <sub><b>so1ar</b></sub></a>
      <br />
      <sub><b>126 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/etiennec78"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15912311/medium/49bacd7fcfc807ea0c5f97127af5bb26.jpeg" />
        <br />
        <sub><b>etiennec79 (etiennec78)</b></sub></a>
      <br />
      <sub><b>114 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/james34602"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15764811/medium/9c166ce647706bbfc568c13763b8a50d.jpeg" />
        <br />
        <sub><b>James Fung (james34602)</b></sub></a>
      <br />
      <sub><b>72 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/arifesat"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15670651/medium/46177c3d13c90ed767700bb49413107f.jpeg" />
        <br />
        <sub><b>Arif Esat Yılmaz (arifesat)</b></sub></a>
      <br />
      <sub><b>24 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/andmydignity"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15821563/medium/cce9327c6cd8879f307495fab2077633.png" />
        <br />
        <sub><b>Semih Aslan (andmydignity)</b></sub></a>
      <br />
      <sub><b>15 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/deathrobert2010"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/13559998/medium/429e149d92ed6c461f601e7d30d280df.jpg" />
        <br />
        <sub><b>Robert Abreu (deathrobert2010)</b></sub></a>
      <br />
      <sub><b>13 words</b></sub>
    </td>
  </tr>
  <tr>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/artemgrebennikov310"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/13820153/medium/d1565ef34bd253f67a995f1fd3811887.jpg" />
        <br />
        <sub><b>Mr.Positiv (artemgrebennikov310)</b></sub></a>
      <br />
      <sub><b>4 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/dev_trace"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15729737/medium/f515d9ef1eeb393759e7180bc700afc2_default.png" />
        <br />
        <sub><b>dev_trace</b></sub></a>
      <br />
      <sub><b>2 words</b></sub>
    </td>
    <td align="center" valign="top">
      <a href="https://crowdin.com/profile/alfredo30.herz"><img alt="logo" style="width: 64px" src="https://crowdin-static.downloads.crowdin.com/avatar/15856703/medium/324d9cf3874ae10cd3efa5ace74a4c89.png" />
        <br />
        <sub><b>Alfredo Hernandez (alfredo30.herz)</b></sub></a>
      <br />
      <sub><b>2 words</b></sub>
    </td>
  </tr>
</table><a href="https://crowdin.com/project/jdsp4linux" target="_blank">Translate in Crowdin 🚀</a>
<!-- CROWDIN-CONTRIBUTORS-END -->

## License

This project is licensed under [GPLv3](LICENSE).

```
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR 
THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
