# JDSP4Linux
Port of the opensource version of JamesDSP to Linux

>**Currently maintained by [@ThePBone](https://t.me/ThePBone)**

This is experimental and still work in progress.
Every feature from the opensource version is implemented.
Feel free to join our [Telegram Group](https://t.me/joinchat/FTKC2A2bolHkFAyO-fuPjw) (combined with Viper4Linux)

You can find the repo of my gst-wrapper for JDSP [here](https://github.com/Audio4Linux/gst-plugin-jamesdsp).

__And the GUI is available [here](https://github.com/ThePBone/JDSP4Linux-GUI)__
## Notes
Since the gst interface of this port is very similar to viper4linux, I will temporarily use a slightly modified version of the [viper script](https://github.com/noahbliss/Viper4Linux/blob/master/viper) written by [@noahbliss](https://github.com/noahbliss).

At the moment JDSP and Viper4Linux are not compatible. Trying to start them both will cause them to interfere with each other and result in no audio output.

For now, I will only include the basics in the readme. More details will be added later.

### Technical Data
Supported sample formats:
* 32-bit float (LE)
* 32-bit int (LE)

Supported samplerates:
* 44100
* 48000
### Effects
Pretty much everything from the opensource version is implemented:
* Analog modelling (12AX7)
* BS2B
* ViPER DDCs
* Limiter
* Compression
* Convolver
* Reverbation (Progenitor2)
* Bass boost
* Stereo widener (Side/Mid) 

Instead of being tied to presets; stereo widener, bs2b and the reverbation engine can be fully customized.

## Setup

### Arch

On Arch you can install the [AUR package](https://aur.archlinux.org/packages/jdsp4linux-git/) by yochananmarqos:
```bash
yay -S jdsp4linux-git
```
And the GUI for JDSP on Linux:
```bash
yay -S jdsp4linux-gui-git
```
Please check the [workarounds section](#workarounds) if you encounter any problems.

![AUR version](https://img.shields.io/aur/version/jdsp4linux-git?label=aur)

### Manually
Install gstreamer-1.0 and some tools to build the gstreamer plugin. (If you have already installed Viper4Linux before, you can skip this step...)

#### Debian
```bash
sudo apt-get install libsamplerate0 libsamplerate0-dev libsndfile1 libsndfile1-dev
sudo apt-get install build-essential autoconf libtool
sudo apt-get install gstreamer-1.0 libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev 
```
#### Arch
```bash
sudo pacman -S base-devel  
sudo pacman -S libsndfile libsamplerate
sudo pacman -S gst-plugins-good  
```
Clone both JDSP4Linux repos
```bash
git clone https://github.com/ThePBone/JDSP4Linux.git
git clone https://github.com/ThePBone/gst-plugin-jamesdsp.git
```
Build the gst plugin
```bash
cd gst-plugin-jamesdsp  
./autogen.sh  
make
```
and install it...
#### Debian
```bash
sudo cp src/.libs/libgstjdspfx.so /usr/lib/x86_64-linux-gnu/gstreamer-1.0/  
```
#### Arch
```bash
sudo cp src/.libs/libgstjdspfx.so /usr/lib/gstreamer-1.0/  
```
If everything went alright gst-inspect-1.0 should display all element properties of the JDSP plugin:
```bash
gst-inspect-1.0 jdspfx
```
Copy the test-script (used to launch JDSP) to /usr/local/bin
```bash
sudo cp ../JDSP4Linux/jdsp /usr/local/bin
```
Create the config folder...
```bash
mkdir -p ~/.config/jamesdsp
```
and copy the default config in there.
```bash
cp ../JDSP4Linux/audio.conf ~/.config/jamesdsp/audio.conf
```
Now JDSP4Linux should be setup correctly.
Edit the audio.conf to your liking and launch JDSP using the script (like Viper4Linux):
```bash
jdsp start
```
And stop it like this:
```bash
jdsp stop
```
Like I said, work-in-progress != stable, so use it with caution. 

> __You can find [installation instructions](https://github.com/ThePBone/JDSP4Linux-GUI) for the GUI [here](https://github.com/ThePBone/JDSP4Linux-GUI)__

> __Join our [Telegram Group](https://t.me/joinchat/FTKC2A2bolHkFAyO-fuPjw)__

## Workarounds
### Error: no element "device"
As described [here](https://github.com/noahbliss/Viper4Linux#configuration), you can set a custom alsa path.
You can also follow the instructions over there, but make sure you drop the devices.conf in `~/.config/jamesdsp`.

##### Short summary:
Run this command:
```bash
pactl list sinks | grep "Name: " -A1
```
...and look for the Alsa Output Path of your headset.
If we have this command output for example:
```bash
Name: alsa_output.pci-0000_00_1f.3.analog-stereo
Description: Built-in Audio Analog Stereo
Name: jdsp
Description: JDSP4Linux
```
Then `alsa_output.pci-0000_00_1f.3.analog-stereo` would be the output path.

Finally create the file `~/.config/jamesdsp/devices.conf` and paste the path in there, like this:
```bash
location=...insert custom alsa path here...
```
...and reload JDSP:
```bash
jdsp restart
```
### Fix crackling/choppy sound
#### Solution 1
Edit `/etc/pulse/default.pa` as root and uncomment or add the following line:

    load-module module-udev-detect tsched=0

#### Solution 2
_Set the default samplerate to 48000Hz in pulseaudio's config:_

`sudo nano /etc/pulse/daemon.conf`

Replace this line:
`;  default-sample-rate = 44100`
with this one:
`default-sample-rate = 48000`

And restart pulseaudio:
```bash
pulseaudio -k
pulseaudio --start
```

## Credits
* [yochananmarqos - AUR packages](https://github.com/yochananmarqos)
* [james34602 - JamesDSP audio engine](https://github.com/james34602/)

______________
Please open an issue or contact me on Telegram @ThePBone if you find any bugs or have questions.
