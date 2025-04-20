# Voicemeeter-SDK
Voicemeeter Remote API + Source Code Examples

Voicemeeter Remote API provides a set of functions to control Voicemeeter parameters,  to process audio inside Voicemeeter, to get MIDI messages coming from the Voicemeeter MIDI-Mapping and to control the MacroButtons application. Voicemeeter SDK offers different source code example. example0 is expected to show every functionnalties while other projects are concrete application examples:

- Matrix8x8: example of Audio Processing Application offering a 8x8 gain matrix on a selected BUS.
- vmr_osd: Exemple of Overlay Screen Display Application to show the current gain of the current moving slider.
- vmr_play: example to use voicemeeter as audio board to playback a stereo sound.
- vmr_streamer: example of custom Graphic User Interface controlling Voicemeeter.

# Compilation Instructions and Documentation
Voicemeeter API are provided by standard Windows DLL installed with Voicemeeter. It provides 2x DLL for x32 and x64 applications. Linking method is given in the source code example. Compilation instructions are given in source code header and all API specifications are in the PDF document.

# Licensing
All source code of this SDK is free to use in any kind of project interacting with Voicemeeter through the voicemeeterremote DLL or VBAN protocol. More details in VoicemeeterRemoteAPI.pdf

Copyright (c) 2021 Vincent Burel

# Links
- Download Voicemeeter: www.voicemeeter.com
- VBAN Protocol: https://vb-audio.com/Voicemeeter/vban.htm
- VB-Audio Support page: https://vb-audio.com/Services/support.htm

# Voicemeeter Plugin Host

A plugin host for audio processing in the Voicemeeter virtual mixer.

## Development Setup

### Windows

1. Ensure PowerShell is installed (comes pre-installed with Windows 10/11)

   If you need to install or update PowerShell:
   ```powershell
   # Install PowerShell 7+ (recommended)
   # Visit https://aka.ms/powershell-download for the latest installer
   
   # Option 1: Install via winget
   winget install Microsoft.PowerShell
   
   # Option 2: Install via direct download
   Invoke-WebRequest -Uri https://github.com/PowerShell/PowerShell/releases/download/v7.3.6/PowerShell-7.3.6-win-x64.msi -OutFile PowerShell-7.3.6-win-x64.msi
   Start-Process msiexec.exe -ArgumentList '/i PowerShell-7.3.6-win-x64.msi /quiet' -Wait
   
   # Option 3: Install via Chocolatey
   # (If you have Chocolatey installed)
   choco install powershell-core
   ```

2. Verify PowerShell installation:
   ```powershell
   pwsh --version
   # or for Windows PowerShell
   powershell -Command "$PSVersionTable.PSVersion"
   ```

3. Run the setup script as Administrator:

```powershell
# Run PowerShell as Administrator
.\setup_dev_env.ps1
```

This script will install:
- Visual Studio Build Tools
- CMake
- Ninja
- Git
- 7-Zip
- VST3 SDK

### Linux/macOS

1. Run the setup script:

```bash
# You might need to run with sudo on Linux
bash ./setup_dev_env.sh
```

This script will install:
- CMake
- Ninja
- Git
- VST3 SDK
- Required development libraries

### Manual Setup

If the scripts don't work for your environment, here's what you need:

1. **C++ Compiler**:
   - Windows: Visual Studio 2019/2022 with C++ workload
   - macOS: Xcode Command Line Tools
   - Linux: GCC or Clang

2. **Build System**:
   - CMake 3.14 or higher
   - Ninja (optional but recommended)

3. **Dependencies**:
   - VST3 SDK 3.7.5+
   - For Linux: X11 and ALSA development libraries

4. **Environment Variables**:
   - Set `VST3_SDK_PATH` to the location of the VST3 SDK

## Building

### Using CMake

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

### Using Visual Studio

Open the generated solution file in the `build` directory after running the setup script.
