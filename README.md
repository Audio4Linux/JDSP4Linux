# JamesDSP (Cross-platform Audio Effect / Digital Signal Processing library)
GUI is based on Omnirom DSP Manager and able to run on most Android devices from 5 to 10 include Samsung, AOSP, Cyanogenmod, recent HTC and Huawei(arm64). 
This app include many cool features.

This repo is a Android variant of JamesDSP, [JDSP4Linux](https://github.com/Audio4Linux/JDSP4Linux) uses core library from current repo.

##### Features:

1. Auto dynamic range compression
   --> A highly automated multiband dynamic range adjusting effect

2. Auto Bass Boost
   --> Frequency detecting bass boost. Effect detect interesting frequency, and adjust gain, bandwidth and cut-off frequency arccordingly

3. Reverb
   --> Progenitor 2 (Complicated IIR network)

4. Interpolating FIR Equalizer

5. Partitioned Convolver (Auto segmenting convolution)
   --> Support mono, stereo, full/true stereo(LL, LR, RL, RR) IR

6. Live programmable DSP
   --> A effect that can compile EEL code into opcode, and do processing base on the compiled code.
The EEL virtual machine had pre-built tons of advanced math routines and DSP function.
Including: Basic C String manipulation functions, Linear algebra solver(Least square, inv(), pinv()), Mathematical optimization(linprog(), quadprog(), lsqlin()), polynomial roots solver(roots()), spectral processing(Short-time Fourier Transform), Polyphase filterbank(Constant Q Transform), multi-purpose IIR / FIR filter designer(eqnerror() / firls()), IIR Subbands transform, Direct form FIR filter, Fractional delay line, Polyphase resampler, real time 1D convolution, Autoregressive Burg estimator, simple peak finding algorithm.

More details:[EEL2 open source variant](https://github.com/james34602/EEL_CLI)

7. Stereo Widen
   --> Algorithm detect stereo phase relation in a few spectral region, and enhance the stereo soundstage without affect vocal integrity

8. Crossfeed
   --> Include traditional BS2B mode and convolution-based HRTF

9. Vacuum tube modelling

10. Viper DDC (IIR Cascaded Second-Order Sections Form II)

##### Supported bit depth:

| # bits   | Status      |
|----------|-------------|
| 8        | Unsupported |
| 16       | Supported   |
| 24(Int)  | Unsupported |
| 32(Int)  | Supported   |
| 32(Float)| Supported   |

## Important
### FAQ
#### 1. Computation datatype?

A: Float32.

#### 2. What is convolver?

A: Convolver is a effect apply convolution(a mathematical operation) on input signal, that perfectly apply user desired response on music, it could simulate physical space.

   Effect itself require audio file(.wav/.irs/.flac) to become impulse response source.

   For more info: [Convolution](https://en.wikipedia.org/wiki/Convolution) and [Convolution reverb](https://en.wikipedia.org/wiki/Convolution_reverb)

#### 3. Installation method

A: Advanced method(Manual installation)

   Effect may get unloaded by Android system if no audio stream for while.

  audio_effects.conf is a file specified for system to load effect using known UUID.
  1. you need to add
   ```
  jdsp {
    path /system/lib/soundfx/libjamesdsp.so
  }
   ```
   ### under
   ```
   bundle {
    path /system/lib/soundfx/libbundlewrapper.so
  }
   ```
   ### AND
   ```
   jamesdsp {
    library jdsp
    uuid f27317f4-c984-4de6-9a90-545759495bf2
  }
   ```
   ### under
   ```
   effects {
   ```
   2. copy libjamesdsp.so to /system/lib/soundfx
   3. Reboot
   4. Install APK

B: Automatic installation(Supported up to Android Pie)

   Join Telegram group [JDSP and V4A group](https://t.me/jDSP_V4A) to receive latest update.
   Methods:
   1. Copy zip package to your phone, reboot to custom recovery, install package, reboot, done.
   2. Get Magisk manager to install JDSP.
Now work on most Android device from Lollipop to Pie

## Download Link
1. See my project release page

# Screenshot
1. [Equalizer screenshot(Dark theme)](https://github.com/james34602/JamesDSPManager/blob/master/ScreenshotMainApp1.png)
2. [Convolver screenshot(Idea theme)](https://github.com/james34602/JamesDSPManager/blob/master/ScreenshotMainApp2.png)

# Important
Modify SELinux is not required(in most case), let your device become safer.
Also, it's good to customizing your own ROM or even port ROM with JamesDSP.
Some device does require SELinux workaround to work correctly

# Contact
Better contact me by email. Send to james34602@gmail.com

# Terms and Conditions / License
The engine frame is based on Antti S. Lankila's DSPManager.

### Credit
1. Joseph Young (Impulse response provider)
2. Christopher Blomeyer (Very patient app tester and inspiring me bit depth issue)
3. [ahrion](https://github.com/therealahrion) (Making installation tools)
4. [Zackptg5](https://github.com/Zackptg5) (Making installation tools)

#### More Credit
1. Matlab, a great tool that do all sort of modelling
