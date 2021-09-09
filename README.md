<h1 align="center">
  <img alt="Icon" width="75" src="https://github.com/Audio4Linux/JDSP4Linux/blob/refactor/resources/icons/icon.png?raw=true">
  <br>
  JamesDSP for Linux
  <br>
</h1>
<h4 align="center">Open-source sound effects for PipeWire and PulseAudio</h4>
<p align="center">
  <a href="https://github.com/ThePBone/DDCToolbox/releases">
  	<img alt="GitHub release (latest by date)" src="https://img.shields.io/github/v/release/thepbone/DDCToolbox">
  </a>
  <a href="https://github.com/ThePBone/DDCToolbox/blob/master/LICENSE">
      <img alt="License" src="https://img.shields.io/github/license/thepbone/DDCToolbox">
  </a>
  <a href="https://github.com/ThePBone/DDCToolbox/">
    <img alt="Windows build" src="https://img.shields.io/github/repo-size/thepbone/ddctoolbox">
  </a>
</p>
<p align="center">
  <a href="#features">Features</a> •
  <a href="#installation">Installation</a> •
  <a href="#contributors">Contributors</a> •
  <a href="#license">License</a> 
</p>

<p align="center">
   <img alt="Screenshot" width="702" src="https://github.com/Audio4Linux/JDSP4Linux/blob/refactor/screenshot.png?raw=true">
</p>


<p align="center">
Currently developed by <a href="https://github.com/thepbone">@thepbone</a> (<a href="https://t.me/thepbone">Telegram</a>)
<p/><p align="center">
<a href="https://github.com/james34602/JamesDSPManager">JamesDSP</a> was initially published as an audio effects processor<br>for Android devices and is written by <a href="https://github.com/james34602">James Fung (@james34602)</a>.
</p>



## Features

* Automatic bass boost
  * Frequency-detecting bass-boost. Automatically sets its own parameters, such as gain, bandwidth, and cut-off frequency by analysing the incoming audio stream
* Automatic dynamic range compressor
  * A highly automated multiband dynamic range adjusting effect
* Complex reverberation IIR network (Progenitor 2)
* Interpolated FIR equalizer with flexible bands
* Arbitrary response equalizer (also known as GraphicEQ from EqualizerAPO)
* Partitioned convolver (Auto segmenting convolution)
  * Supports mono, stereo, full/true stereo (LL, LR, RL, RR) impulse response
* Crossfeed
  * Realistic surround effects
* Soundstage wideness
  * A multiband stereo wideness controller
* ViPER-DDC
  * Perform parametric equalization on audio
  * Create VDC input files using [thepbone/DDCToolbox](https://github.com/thepbone/DDCToolbox)
* Analog modelling
  * An aliasing-free even harmonic generator
* Output limiter
* **Scripting engine: Live programmable DSP**
  * Write your own audio effects using the [EEL2 scripting language](https://github.com/james34602/EEL_CLI)
  * Auto-generate a basic user interface for your script to modify certain parameters/constants without editing the actual code
  * The scripting language has been extended using many DSP-related functions for easy access, for example: spectral processing, constant Q transform, multi-purpose FIR filter designer IIR sub-bands transformation, etc...
  * **This app also includes a custom minimal scripting IDE**:
    * Syntax highlighting
    * Basic code completion
    * Dynamic code outline window
    * Console output support
    * Detailed error messages with inline code highlighting



## PipeWire vs PulseAudio

Designed for use with PipeWire. PulseAudio is primarily supported for backward-compatibility. 

TBD

## Installation

* [PPA (Debian/Ubuntu)](#debianubuntu)
* [AUR (Arch Linux)](#arch)
* [Build from sources](#build-from-sources)

### Debian/Ubuntu
Add PPA Repo
```bash
curl -s --compressed "https://thepbone.github.io/PPA-Repository/KEY.gpg" | sudo apt-key add -
sudo curl -s --compressed -o /etc/apt/sources.list.d/thepbone_ppa.list "https://thepbone.github.io/PPA-Repository/thepbone_ppa.list"
sudo apt update
```
Install from PPA

For PipeWire clients only (recommended):
```bash
sudo apt install jamesdsp-pipewire
```
For PulseAudio clients only:
```bash
sudo apt install jamesdsp-pulse
```
[View PPA on GitHub](https://github.com/ThePBone/PPA-Repository)


### Arch
TBD

On Arch you can install the [AUR package](https://aur.archlinux.org/packages/jdsp4linux-gui-git/) by yochananmarqos:
```bash
yay -S jdsp4linux-gui-git
```

Or [this one](https://aur.archlinux.org/packages/jdsp4linux-gui) to get the latest stable release:
```bash
yay -S jdsp4linux-gui
```
![AUR version](https://img.shields.io/aur/version/jdsp4linux-gui?label=aur%20%28stable%29) ![AUR version](https://img.shields.io/aur/version/jdsp4linux-gui-git?label=aur%20%28git%29)


### Build from sources

**Requirements:**

 * Qt 5.11 or later

Install dependencies (Debian)

    sudo apt install qt5-qmake qtbase5-dev libgl1-mesa-dev

Install dependencies (Arch)

    sudo pacman -S qt5-base 

Clone this repository

    git clone https://github.com/Audio4Linux/JDSP4Linux

Compile sources

    cd JDSP4Linux
    qmake JDSP4Linux.pro
    make

Execute compiled binary

```bash
./jamesdsp
```

#### Optional: Manual installation

Copy binary to /usr/local/bin

```bash
sudo cp jamesdsp /usr/local/bin
sudo chmod 755 /usr/local/bin/jamesdsp
```

Create a menu entry

```bash
sudo cat <<EOT >> /usr/share/applications/jamesdsp.desktop
[Desktop Entry]
Name=JamesDSP
GenericName=Equalizer
Comment=
Keywords=equalizer
Categories=AudioVideo;Audio;
Exec=jdsp-gui
Icon=/usr/share/pixmaps/jamesdsp.png
StartupNotify=false
Terminal=false
Type=Application
EOT
```

Download icon

```bash
sudo wget -O /usr/share/pixmaps/jamesdsp.png https://raw.githubusercontent.com/Audio4Linux/JDSP4Linux-GUI/master/resources/icons/icon.png -q --show-progress
```

## Screenshots

TBD

## Contributors

* [James Fung](https://github.com/james34602) - Developer of the core library ['libjamesdsp'](https://github.com/james34602/JamesDSPManager/tree/master/Main)
* [yochananmarqos](https://github.com/yochananmarqos) - AUR packages

##### Other credits
* PipeWire implementation based on [EasyEffects](https://github.com/wwmm/EasyEffects)

## License

This project is licensed under [GPLv3](LICENSE).

```
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR 
THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
