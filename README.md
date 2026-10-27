# NeoDyn

Sintetizador VST3 con motor wavetable + unison (estilo Serum/Massive/Sylenth1) y un módulo de **guitarra acústica** (modelado físico Karplus-Strong con modo strum) para sonidos tipo Avicii / folk-EDM.

## Qué incluye
- 2 osciladores wavetable (8 formas con morphing: seno, triangular, saw, cuadrada, pulso, formante...), anti-aliasing por mipmaps
- Unison hasta 7 voces con detune y stereo spread (supersaw)
- Filtro LP / BP / HP con resonancia, envolvente de filtro y LFO (a wave y cutoff)
- Envolvente de amplitud
- Guitarra acústica: level, decay, brillo y **strum** (los acordes se rasguean de grave a agudo)
- FX: chorus, delay, reverb
- 7 presets (Supersaw Lead, Pluck Trance, Folk Guitar, Avicii Layer, Wobble Bass, Pad...)

## Opción A: sin instalar nada (GitHub Actions)
1. Crea un repositorio en GitHub y sube todo el contenido de esta carpeta (incluida `.github`).
2. Ve a la pestaña **Actions**. El build corre solo (o pulsa "Run workflow").
3. Cuando termine (~10-15 min), entra al run y descarga el artefacto `NeoDyn-VST3-windows-latest` o `...-macos-latest`.
4. Descomprime y copia `NeoDyn.vst3`:
   - **Windows:** `C:\Program Files\Common Files\VST3\`
   - **macOS:** `~/Library/Audio/Plug-Ins/VST3/`
5. En macOS, si no carga: `xattr -cr ~/Library/Audio/Plug-Ins/VST3/NeoDyn.vst3`
6. Abre tu DAW y re-escanea plugins.

## Opción B: compilar en tu PC
Requisitos: CMake 3.22+, Git y un compilador (Visual Studio 2022 en Windows, Xcode en macOS).

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target NeoDyn_VST3
```
El plugin queda en `build/NeoDyn_artefacts/Release/VST3/NeoDyn.vst3`.
Para que se copie solo a la carpeta del sistema: añade `-DNEODYN_INSTALL=ON` (en Windows, ejecuta la terminal como administrador).

También se genera una versión Standalone (app independiente) para probarlo sin DAW.

## Cómo usarlo para sonidos tipo Avicii
1. Elige el preset **Avicii Layer** o **Folk Guitar**.
2. Toca acordes: con `Strum ms` > 0 se rasguean.
3. Mezcla `Synth` y `Guitar Level`, y sube `Rev Mix` / `Dly Mix` al gusto.

## Licencia
JUCE se distribuye bajo AGPLv3 o licencia comercial. Para uso personal está bien; si vas a vender o distribuir el plugin, revisa su licencia: https://juce.com/juce-8-licence/
