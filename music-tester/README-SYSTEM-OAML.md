# Using System-Installed OAML with Music Tester

This version of the music-tester application has been simplified to use the system-installed OAML library instead of building it from source or using patches.

## Prerequisites

The OAML library must be installed on your system. As shown in your output, it should be available at:
- `/usr/local/lib/liboaml.a`
- `/usr/local/lib/liboaml.1.3.4.dylib`
- `/usr/local/lib/liboaml.1.dylib`
- `/usr/local/lib/liboaml.dylib`
- `/usr/local/include/oaml.h`

## Building the Music Tester

1. Run the build script:
```bash
./music-tester/build-macos.sh
```

The script will:
- Check for required system libraries (libogg, libvorbis, pkg-config)
- Check for OAML (warning you if it's not found via Homebrew)
- Install minizip via vcpkg in classic mode
- Configure and build the music-tester application

If you need to clean and rebuild, use:
```bash
./music-tester/build-macos.sh clean
```

## Running the Music Tester

After building, run the application with:
```bash
./build/music-tester
```

By default, it will look for audio files in the `sound_staging/` directory. You can specify a different directory as a command-line argument:
```bash
./build/music-tester /path/to/audio/files
```

## OAML Integration

The music-tester uses a simple XML configuration file (`music-tester.defs`) to set up the OAML audio engine. This file is automatically copied to the build directory when the application is built.

## Troubleshooting

If the music-tester fails to run with errors about missing OAML libraries:

1. Verify that OAML is properly installed:
```bash
ls -la /usr/local/lib/liboaml*
```

2. Make sure the OAML library can be found by the linker:
```bash
echo $LD_LIBRARY_PATH
```

3. On macOS, you might need to add the OAML library path to DYLD_LIBRARY_PATH:
```bash
export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH
```

4. If you built OAML from source, ensure it was installed correctly:
```bash
cd /path/to/oaml/build
sudo make install
```