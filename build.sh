#!/bin/bash

# =========================================================
# Emscripten SDK (emsdk) Kurulumu
# =========================================================
# emsdk repo'sunu klonla veya güncelle
if [ ! -d "emsdk" ]; then
    git clone https://github.com/emscripten-core/emsdk.git
else
    (cd emsdk && git pull)
fi

# 'latest' SDK'yı indir ve kur
./emsdk/emsdk install latest
./emsdk/emsdk activate latest

# EMSDK ortam değişkenlerini ayarla
source "./emsdk/emsdk_env.sh"

# =========================================================
# Derleme Komutu
# =========================================================

# motor_core.cpp dosyasını derle
# Gerekli Bayraklar:
# 1. --js-library js_api.js: C++'tan çağrılan JS fonksiyonlarının tanımlarını (stubs) sağlar.
# 2. -s EXPORTED_FUNCTIONS: C-style olarak dışa aktarılacak fonksiyonları belirtir. (Önek olarak "_" unutulmamalıdır.)
# 3. --bind: Emscripten C++ bindings (embind) kullanımını etkinleştirir.

emcc motor_core.cpp \
    --js-library js_api.js \
    -o motor.js \
    -s MODULARIZE=1 \
    -s EXPORT_ES6=1 \
    -s EXPORT_NAME=Module \
    -s INITIAL_MEMORY=32MB \
    -s EXPORTED_FUNCTIONS='_RunScript,_InitializeEngine,_CreateNewBlockFromUI' \
    --bind \
    -O2 
