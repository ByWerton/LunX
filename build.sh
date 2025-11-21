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
# ÖNEMLİ DÜZELTME: EXPORTED_FUNCTIONS listesi kaldırıldı. 
# Tüm public fonksiyonlar embind ile dışa aktarıldığı için 
# bu liste çakışmaya neden oluyordu.

emcc motor_core.cpp \
    --js-library js_api.js \
    -o motor.js \
    -s MODULARIZE=1 \
    -s EXPORT_ES6=1 \
    -s EXPORT_NAME=Module \
    -s INITIAL_MEMORY=32MB \
    --bind \
    -O2 
