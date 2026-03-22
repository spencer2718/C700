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

Copy or symlink the .vst3 bundle into your VST3 search path:

```bash
mkdir -p ~/.vst3
cp -r build/C700_artefacts/Release/VST3/C700.vst3 ~/.vst3/
```

Then rescan plugins in REAPER.
