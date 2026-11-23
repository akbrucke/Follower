![Static Badge](https://img.shields.io/badge/Version-1.00-white) 
# Follower
<p align="center">
   <img width="656" height="131" alt="FollowerLogo" src="https://github.com/user-attachments/assets/0b0b14ee-461a-44bb-9a54-a3c3c9762ec4" />  
</p>
VST3 component-modeled amp sim suite using LiveSPICE and JUCE Framework, vibe-coded and then refined with a human touch   

## Overview & Design Philosophy
Follower is a complete guitar amplifier and effects workstation engineered to recreate the monolithic, atmospheric guitar tones of Cult of Luna's landmark album Salvation. Rather than isolating a single amplifier in a sterile studio chain, Follower models the entire signal path - from boutique op-amp overdrive and pre-amplifier modulation, delay, and reverb straight into two switchable, volume-matched 100-watt tube heads, bespoke cabinet IRs shot by **INSERT PERSON HERE**, and a true stereo post-cabinet tape echo.  

A defining element of the Salvation sound is running time-based effects ahead of high-headroom, saturating tube preamps so that tremolo swells, analog delay repeats, and cavernous reverb tails bloom organically into power-tube compression. Follower pairs this pre-amp pedalboard with independent parameter recall for both the Trainer Mark III and Rocker 100 amp heads, plus three collapsible drawers that keep the workspace compact until you need to tweak. 

## Features

### Pedals
**Overdrive**: In front of the amps resides a component-modeled rare boutique Swedish overdrive pedal that was key to the guitar sound on Salvation. This circuit has been modded with two additional controls: **Fatness** and **Voltage**.
Even without these mods, this rare overdrive is special when combined with the amps it feeds.  

Also in front of the amp (in true post-metal fashion) are three time-based effects: **Tremolo**, **Analog Delay**, and **Hall Reverb**. Using these effects before the amps on high gain settings creates huge swells of lo-fi ambience, and using them in front of the amps set clean creates beautiful and delicate tones that feel amazing to play.  

### Amps
Two 100-watt heads, component-modeled with LiveSPICE. Every control works just as it would on the real amps. Amp 1: **Trainer Mark III*** - a Canadian-made all-tube 2x12 combo offering loud, punchy cleans with fuller midrange than a typical high-watt 2x12 combo, and wonderful breakup when pushed with the overdrive pedal. It features a Treble Boost switch that compliments the tone perfectly. Amp 2: **Rocker 100*** - a British 2-channel all-tube beast; much warmer and rounder sounding than the Trainer. Capable of delivering full-on fuzzy doom metal rhythm tones when combined with the overdrive pedal (and its Fatness mod.) The clean channel reacts amazingly with the pre & post FX.

### Cabinets & IR Loader
The plugin features four bespoke cabinet IRs shot by **INSERT PERSON HERE** specifically for Follower. Cabinets range from mid-forward to biting to warm to bass-heavy.  

Follower also has a built-in IR loader for use with external IRs, as well as a bypass, which disables cabinet simulation with automatic −12 dB level compensation for external IR loaders.

### Stereo Tape Echo
After the cabinet, in true stereo, lives an implementation of the incredible Tape Echo by Dusk Audio. This open source 3-head tape echo and spring reverb engine is the final piece of the puzzle in Follower, adding warm, lush width and dimension to its already huge post-metal tonality. This section of the plugin is easy to get lost in - be warned!  

*Licensing notice: Tape Echo by Dusk Audio is released under the GPLv3 license, the same license that Follower is released under.*  

### Utility Features
Lastly, Follower includes an effective noise gate, with a toggle for fast or slow release, and high-pass/low-pass filters, for getting mix-ready tones.  

## Screenshots  

**INSERT SCREENSHOTS HERE**

## Installation
Currently, Follower is released for Windows 10/11 only, but Mac and Linux releases are coming soon. To grab the latest prebuilt release, head to the [releases page.](https://github.com/akbrucke/Follower/releases) Included with the release download is a thorough user manual, a collection of presets, and installation instructions.

## Building from Source (Windows)

Maple uses CMake and automatically fetches JUCE (`9.0.5`) during configuration. No manual SDK downloads or git submodules are required.

### Prerequisites
* **Git**
* **CMake 3.22 or newer**
* **Visual Studio 2022** (Community Edition or newer) with the **Desktop development with C++** workload installed

### Build Steps

1. Clone the repository
   
2. Open a terminal in the root folder and run  
```cmake -B build -G "Visual Studio 17 2022" -A x64```  
And then compile the release build:  
```cmake --build build --config Release```

3. Locate & Install the VST3 Plugin  
The finished VST3 bundle will be located in ```build\Follower_artefacts\Release\VST3\Follower.vst3```  
Copy ```Follower.vst3``` into your system VST3 folder so your DAW can detect it.  
On Windows, this would be: ```C:\Program Files\Common Files\VST3\```

## Changelog
FOLLOWER V1.00 (INSERT DATE HERE, 2026)  
-Released

