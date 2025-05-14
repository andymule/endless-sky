# Endless Sky

Explore other star systems. Earn money by trading, carrying passengers, or completing missions. Use your earnings to buy a better ship or to upgrade the weapons and engines on your current one. Blow up pirates. Take sides in a civil war. Or leave human space behind and hope to find some friendly aliens whose culture is more civilized than your own...

------

Endless Sky is a sandbox-style space exploration game similar to Elite, Escape Velocity, or Star Control. You start out as the captain of a tiny spaceship and can choose what to do from there. The game includes a major plot line and many minor missions, but you can choose whether you want to play through the plot or strike out on your own as a merchant or bounty hunter or explorer.

See the [player's manual](https://github.com/endless-sky/endless-sky/wiki/PlayersManual) for more information, or the [home page](https://endless-sky.github.io/) for screenshots and the occasional blog post.

## Installing the game

Official releases of Endless Sky are available as direct downloads from [GitHub](https://github.com/endless-sky/endless-sky/releases/latest), on [Steam](https://store.steampowered.com/app/404410/Endless_Sky/), on [GOG](https://gog.com/game/endless_sky), and on [Flathub](https://flathub.org/apps/details/io.github.endless_sky.endless_sky). Other package managers may also include the game, though the specific version provided may not be up-to-date.

## System Requirements

Endless Sky has very minimal system requirements, meaning most systems should be able to run the game. The most restrictive requirement is likely that your device must support at least OpenGL 3.

|| Minimum | Recommended |
|---|----:|----:|
|RAM | 750 MB | 2 GB |
|Graphics | OpenGL 3.0 | OpenGL 3.3 |
|Storage Free | 350 MB | 1.5 GB |

## Building from source

Development is done using [CMake](https://cmake.org) to compile the project. Most popular IDEs are supported through their respective CMake integration.

For full installation instructions, consult the [Build Instructions](docs/readme-developer.md) readme.

## Contributing

As a free and open source game, Endless Sky is the product of many people's work. Contributions of artwork, storylines, and other writing are most in-demand, though there is a loosely defined [roadmap](https://github.com/endless-sky/endless-sky/wiki/DevelopmentRoadmap). Those who wish to [contribute](docs/CONTRIBUTING.md) are encouraged to review the [wiki](https://github.com/endless-sky/endless-sky/wiki), and to post in the [community-run Discord](https://discord.gg/ZeuASSx) beforehand. Those who prefer to use Steam can use its [discussion rooms](https://steamcommunity.com/app/404410/discussions/) as well, or GitHub's [discussion zone](https://github.com/endless-sky/endless-sky/discussions).

Endless Sky's main discussion and development area was once [Google Groups](https://groups.google.com/g/endless-sky), but due to factors outside our control, it is now inaccessible to new users, and should not be used anymore.

## Licensing

Endless Sky is a free, open source game. The [source code](https://github.com/endless-sky/endless-sky/) is available under the GPL v3 license, and all the artwork is either public domain or released under a variety of Creative Commons (and similarly permissive) licenses. (To determine the copyright status of any of the artwork, consult the [copyright file](https://github.com/endless-sky/endless-sky/blob/master/copyright).)

## Building from Source

### Linux / BSDs

// ... existing code ...

### macOS (Intel)

// ... existing code ...

### macOS (Apple Silicon / ARM)

Building on Apple Silicon (M1/M2/M3) Macs requires a few specific steps to ensure proper OpenGL support:

#### Requirements

* Xcode Command Line Tools: `xcode-select --install`
* Homebrew (recommended for dependencies): https://brew.sh/
* CMake: `brew install cmake`
* Relevant libraries: `brew install libpng libjpeg-turbo sdl2 openal-soft`

#### Building

1. Clone the repository:
   ```
   git clone https://github.com/endless-sky/endless-sky.git
   cd endless-sky
   ```

2. Configure and build:
   ```
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake -Dunofficial-minizip_DIR=$(pwd)/vcpkg/packages/minizip_arm64-osx/share/unofficial-minizip
   cmake --build build -j$(sysctl -n hw.ncpu)
   ```

   This will:
   - Configure CMake to use vcpkg for dependencies
   - Use the native macOS OpenGL framework rather than X11's OpenGL libraries
   - Build using all available CPU cores

3. Run the game:
   ```
   ./build/endless-sky
   ```

#### Troubleshooting

If you encounter OpenGL rendering issues, ensure you're using the native macOS OpenGL framework and not X11's implementation. This is handled automatically in the CMake configuration provided above.

### Windows (MSVC)

// ... existing code ...
