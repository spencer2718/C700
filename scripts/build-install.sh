#!/bin/bash
# Build C700 VST3 and install to ~/.vst3 for REAPER
set -e
cd "$(dirname "$0")/.."
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
mkdir -p ~/.vst3
cp -r build/C700_artefacts/Release/VST3/C700.vst3 ~/.vst3/
echo "Installed C700.vst3 to ~/.vst3/"
echo "Rescan plugins in REAPER: Options > Preferences > VST > Re-scan"
