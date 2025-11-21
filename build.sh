#!/bin/bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
git pull
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
cd ..
emcc motor_core.cpp -o motor_core.js -s WASM=1 -s SINGLE_FILE=1 -s EXPORTED_FUNCTIONS='["_RunScript", "_InitializeLunXStudio", "_CreateNewBlockFromUI"]'
if [ -f "motor_core.js" ]; then
exit 0
else
exit 1
fi
