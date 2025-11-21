#!/bin/bash
Netlify Build Ortamında Emscripten SDK'sını kurar ve etkinleştirir.
echo "--- Emscripten SDK Kurulumu Başlatılıyor ---"
1. EMSDK deposunu klonla
git clone https://github.com/emscripten-core/emsdk.git
2. EMSDK dizinine git ve en son sürümü çek
cd emsdk
git pull
3. SDK'yı indir ve etkinleştir
Bu iki komut, emcc'nin bulunması için PATH değişkenini ayarlar.
./emsdk install latest
./emsdk activate latest
4. PATH'i mevcut shell oturumu için ayarla (emsdk_env.sh betiğini çalıştır)
source ./emsdk_env.sh
5. Ana proje dizinine geri dön
cd ..
echo "--- C++ Motoru Derleniyor (motor_core.cpp) ---"
6. Emcc'yi çalıştır
Önceki build.command'ımız şimdi bu betiğin içinde çalışıyor.
emcc motor_core.cpp -o motor_core.js -s WASM=1 -s SINGLE_FILE=1 -s EXPORTED_FUNCTIONS='["_RunScript", "_InitializeLunXStudio", "_CreateNewBlockFromUI"]'
7. Başarılı olup olmadığını kontrol et
if [ -f "motor_core.js" ]; then
echo "SUCCESS: motor_core.js başarıyla oluşturuldu."
else
echo "ERROR: motor_core.js oluşturulamadı! Derleme başarısız."
exit 1
fi
