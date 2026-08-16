# Dynamix for Endless Sky

Standalone ImGui app for authoring adaptive music: multi-stem OGG playback, SoLoud effects, granular tempo (Signalsmith Stretch), and an event-driven JSON song format.

## Build (this machine is Windows)

See **[how-to-windows.md](how-to-windows.md)** for the full Windows workflow.

```bat
install-msys2-deps.bat
build-windows-fast.bat
run-tests.bat
run-dynamix.bat
```

macOS: `./build-macos.sh` then `cd build && ./dynamix`  
Linux: `./build-linux.sh`

## Features

- Load song folders (`_song.json` + `.ogg` stems) and a project-level `_master.json`
- Per-track volume and SoLoud filters; master bus effects
- Tape speed (pitch changes) and granular tempo (pitch-preserving)
- Event transitions with lerping; C API in `src/music_tester_api.h` (see [API_USAGE.md](API_USAGE.md))
- In-app console for JSON validation messages
- Catch2 tests under `tests/` (`-DBUILD_TESTS=ON`)

## Keyboard

- **Space**: Play/Pause
- **1-9 / 0**: Tracks 1-10

## Tempo

- Tape speed: 0.1x-4.0x (affects pitch)
- Granular tempo: 0.5x-2.0x (pitch-preserving, Signalsmith Stretch)

## Song format

```
project/
├── _master.json
└── song-name/
    ├── _song.json
    └── *.ogg
```

Details: [fileformat.md](fileformat.md). Engine integration: [API_USAGE.md](API_USAGE.md).

## Architecture

```
src/
├── main.cpp                 # SDL/OpenGL + C API
├── AudioController.*        # MVC controller
├── AudioSystem.*            # SoLoud, sync, filters
├── AudioStreamProcessor.*   # Granular stretch thread
├── SongManager.*            # JSON + folder loading
├── EventSystem.*            # Event lerping
├── JsonValidator.*          # Song/master JSON checks
├── FilterManager.*          # Effect parameter map
├── MainView.* + Views/      # ImGui
├── ConsoleLog.*             # In-app log drawer
└── music_tester_api.h       # External C API
```

CMake FetchContent (or `.deps/` via `USE_PREDOWNLOADED_DEPS`): ImGui, SoLoud, Signalsmith Stretch, Signalsmith Linear, nlohmann/json. Versions are pinned in `CMakeLists.txt`.

## Tests

```powershell
.\run-tests.bat                              # build with tests, run the [unit] tier
.\build-windows.ps1 -Config Release -Tests   # build only
.\build\dynamix_tests.exe                    # everything: unit, audio, integration, roundtrip
.\build\dynamix_tests.exe [audio]            # one tier
```

Catch2, fetched by CMake when `-DBUILD_TESTS=ON`. The audio tiers mix real audio through SoLoud's null driver, so no audio device is needed.

## License

Part of the Endless Sky ecosystem; same licensing terms.
