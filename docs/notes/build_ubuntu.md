# Building C700 on Ubuntu

## Prerequisites

Ubuntu 22.04+ with:

```bash
sudo apt install -y \
    build-essential \
    cmake \
    libasound2-dev \
    libjack-jackd2-dev \
    libfreetype-dev \
    libfontconfig1-dev \
    libcurl4-openssl-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev
```

## Clone (with submodules)

```bash
git clone --recurse-submodules git@github.com:spencer2718/C700.git
cd C700
```

If already cloned without submodules:

```bash
git submodule update --init --recursive
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Output

The VST3 bundle is at:

```
build/C700_artefacts/Release/VST3/C700.vst3
```

## Install (for REAPER)

The build script handles everything:

```bash
bash scripts/build-install.sh
```

This builds, copies the .vst3 to `~/.vst3/`, and installs `playercode.bin` to `~/.config/C700/` for SPC export support.

Then rescan plugins in REAPER: Options > Preferences > VST > Re-scan.

### Manual install

```bash
mkdir -p ~/.vst3
cp -r build/C700_artefacts/Release/VST3/C700.vst3 ~/.vst3/
mkdir -p ~/.config/C700
cp resources/playercode.bin ~/.config/C700/
```
