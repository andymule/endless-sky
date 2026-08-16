# Dynamix Emscripten WASM Spike

Minimal proof that the Dynamix audio stack compiles to WebAssembly and runs the two riskiest code paths:

1. **Signalsmith Stretch** via `AudioStreamProcessor`
2. **SoLoud NULL backend + `SyncWav`** granular playback (same path the desktop app uses)

This is not the full ImGui/SDL app yet — only the audio core needed to de-risk a browser port.

## Run locally

```bash
cd dynamix/emscripten
./build-and-test.sh
```

The script will:

1. Install Emscripten 3.1.64 into `dynamix/.emsdk` if `emcc` is not already on `PATH`
2. Configure and build `spike.js` + `spike.wasm`
3. Execute the spike with Node and expect exit code `0` plus:

```
PASS: dynamix emscripten wasm spike
```

## What it verifies

| Check | Component |
|-------|-----------|
| Stretch processes non-silent audio at 2× tempo | `AudioStreamProcessor` + Signalsmith |
| Granular `SyncWav` mixes through SoLoud | `SyncWav`, SoLoud NULL driver |

## Next steps toward a browser app

- Add SDL2 + ImGui emscripten target on top of this audio core
- Replace native filesystem browsing with upload/IDBFS
- Switch SoLoud from NULL to SDL2/WebAudio backend for real speaker output
