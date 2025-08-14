#!/bin/bash
set -e

# Parse command line arguments
BUILD_TYPE="Release"
CLEAN_BUILD=false
BUNDLE_SDL2=false
BUILD_STATIC=false

while [[ $# -gt 0 ]]; do
    case $1 in
        clean)
            CLEAN_BUILD=true
            shift
            ;;
        debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        release)
            BUILD_TYPE="Release"
            shift
            ;;
        bundle)
            BUNDLE_SDL2=true
            shift
            ;;
        static)
            BUILD_TYPE="Release"
            BUILD_STATIC=true
            shift
            ;;
        *)
            echo "Usage: $0 [clean] [debug|release] [bundle] [static]"
            echo "  clean   - Clean build directory before building"
            echo "  debug   - Build in Debug mode (default: Release)"
            echo "  release - Build in Release mode"
            echo "  bundle  - Build with SDL2 bundling (self-contained binary)"
            echo "  static  - Build static binary"
            echo ""
            echo "Examples:"
            echo "  $0                    # Normal release build"
            echo "  $0 bundle             # Bundled release build"
            echo "  $0 clean bundle       # Clean bundled build"
            echo "  $0 debug bundle       # Debug bundled build"
            exit 1
            ;;
    esac
done

# Determine architecture
ARCH=$(uname -m)
echo "Detected architecture: $ARCH"
echo "Build type: $BUILD_TYPE"
if [ "$BUNDLE_SDL2" = true ]; then
    echo "SDL2 bundling: ENABLED (self-contained binary)"
else
    echo "SDL2 bundling: DISABLED (requires system SDL2)"
fi
if [ "$BUILD_STATIC" = true ]; then
    echo "Static build: ENABLED"
else
    echo "Static build: DISABLED"
fi

# Get Homebrew prefix for library paths
BREW_PREFIX=$(brew --prefix)
echo "Homebrew prefix: $BREW_PREFIX"

# Set explicit library paths
export LIBRARY_PATH="${BREW_PREFIX}/lib:/usr/local/lib:${LIBRARY_PATH}"
export LD_LIBRARY_PATH="${BREW_PREFIX}/lib:/usr/local/lib:${LD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH="${BREW_PREFIX}/lib:/usr/local/lib:${DYLD_LIBRARY_PATH}"
export PKG_CONFIG_PATH="${BREW_PREFIX}/lib/pkgconfig:${BREW_PREFIX}/opt/libvorbis/lib/pkgconfig:${BREW_PREFIX}/opt/libogg/lib/pkgconfig:${BREW_PREFIX}/opt/libpng/lib/pkgconfig:${BREW_PREFIX}/opt/jpeg/lib/pkgconfig:${BREW_PREFIX}/opt/openal-soft/lib/pkgconfig:/usr/local/lib/pkgconfig:${PKG_CONFIG_PATH}"

# Get the script directory
SCRIPT_DIR="$(dirname "$0")"
cd "$SCRIPT_DIR"
DYNAMIX_DIR=$(pwd)
echo "Dynamix directory: $DYNAMIX_DIR"

# Parent project directory
cd ..
PARENT_DIR=$(pwd)
echo "Parent project directory: $PARENT_DIR"

# Create the build directory in the dynamix folder
BUILD_DIR="$DYNAMIX_DIR/build"

# Set minizip triplet based on architecture
if [ "$ARCH" == "arm64" ]; then
    MINIZIP_TRIPLET="arm64-osx"
else
    MINIZIP_TRIPLET="x64-osx"
fi

# Determine number of CPU cores for parallel builds
CPU_CORES=$(sysctl -n hw.ncpu)
MAKE_JOBS=$((CPU_CORES - 1))
# Ensure at least 1 job
[ "$MAKE_JOBS" -lt 1 ] && MAKE_JOBS=1
echo "Using $MAKE_JOBS parallel jobs for building"

# Clean if explicitly requested
if [ "$CLEAN_BUILD" = true ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Create a timestamp file to track package installs
PACKAGE_TIMESTAMP="$DYNAMIX_DIR/.package_timestamp"

# Check if packages need to be verified (only check once per day)
CURRENT_DATE=$(date +%Y%m%d)
LAST_CHECK_DATE=0
[ -f "$PACKAGE_TIMESTAMP" ] && LAST_CHECK_DATE=$(cat "$PACKAGE_TIMESTAMP")

if [ "$LAST_CHECK_DATE" != "$CURRENT_DATE" ] || [ "$CLEAN_BUILD" = true ]; then
    # Check for required packages
    echo "Checking for required libraries..."
    REQUIRED_PACKAGES=("pkg-config" "sdl2" "libpng" "jpeg" "openal-soft" "cmake" "ninja")
    MISSING_PACKAGES=()
    
    # Check all packages in a single brew call to speed up checks
    INSTALLED_PACKAGES=$(brew list --formula)
    
    for package in "${REQUIRED_PACKAGES[@]}"; do
        if ! echo "$INSTALLED_PACKAGES" | grep -q "^$package\$"; then
            MISSING_PACKAGES+=("$package")
        fi
    done
    
    # Install missing packages if needed
    if [ ${#MISSING_PACKAGES[@]} -gt 0 ]; then
        echo "Installing required system libraries via Homebrew: ${MISSING_PACKAGES[*]}"
        brew install "${MISSING_PACKAGES[@]}"
    fi
    
    # Update timestamp file
    echo "$CURRENT_DATE" > "$PACKAGE_TIMESTAMP"
else
    echo "Skipping package check (checked today already)"
fi

# Check if vcpkg is already set up correctly - if so, skip setup
VCPKG_CONFIGURED=false
echo "Checking for parent vcpkg at: $PARENT_DIR/vcpkg"
if [ -d "$PARENT_DIR/vcpkg" ]; then
    echo "Found parent vcpkg directory"
    VCPKG_DIR="$PARENT_DIR/vcpkg"
    # Check if minizip is already installed
    if [ -d "$VCPKG_DIR/installed/${MINIZIP_TRIPLET}/share/unofficial-minizip" ] || \
       [ -d "$VCPKG_DIR/packages/minizip_${MINIZIP_TRIPLET}/share/unofficial-minizip" ]; then
        echo "minizip already installed, skipping vcpkg configuration"
        VCPKG_CONFIGURED=true
    else
        echo "Using parent project's vcpkg for minizip..."
        # Make sure vcpkg is bootstrapped
        if [ ! -f "$VCPKG_DIR/vcpkg" ]; then
            echo "Bootstrapping vcpkg..."
            cd "$VCPKG_DIR"
            ./bootstrap-vcpkg.sh -disableMetrics
            cd "$DYNAMIX_DIR"
        fi
    fi
else
    echo "Parent project's vcpkg not found. Setting up local vcpkg..."
    
    # Set up vcpkg locally if needed
    if [ ! -d "$DYNAMIX_DIR/vcpkg" ]; then
        cd "$DYNAMIX_DIR"
        git clone --depth=1 https://github.com/microsoft/vcpkg.git vcpkg
        cd "$DYNAMIX_DIR/vcpkg"
        ./bootstrap-vcpkg.sh -disableMetrics
        cd "$DYNAMIX_DIR"
    fi
    
    VCPKG_DIR="$DYNAMIX_DIR/vcpkg"
fi

# Install minizip if not already configured
if [ "$VCPKG_CONFIGURED" = false ]; then
    # Install minizip in classic mode to avoid manifest issues
    echo "Installing minizip with classic mode..."
    "$VCPKG_DIR/vcpkg" install minizip:$MINIZIP_TRIPLET --classic --no-print-usage
fi

# Find minizip directory
MINIZIP_DIR=""
for dir in "$VCPKG_DIR/installed/${MINIZIP_TRIPLET}/share/unofficial-minizip" \
           "$VCPKG_DIR/packages/minizip_${MINIZIP_TRIPLET}/share/unofficial-minizip"; do
    if [ -d "$dir" ]; then
        MINIZIP_DIR="$dir"
        break
    fi
done

if [ -z "$MINIZIP_DIR" ]; then
    echo "ERROR: Could not find minizip. Exiting."
    exit 1
fi
echo "Found minizip at: $MINIZIP_DIR"

# Use Ninja generator if available to speed up builds
NINJA_AVAILABLE=false
if command -v ninja &> /dev/null; then
    NINJA_AVAILABLE=true
    echo "Using Ninja build system for faster builds"
fi

echo "Configuring and building dynamix..."
# Configure with CMake
cd "$BUILD_DIR"

# Only reconfigure if needed
if [ ! -f "$BUILD_DIR/build.ninja" ] && [ ! -f "$BUILD_DIR/Makefile" ] || [ "$CLEAN_BUILD" = true ]; then
    # Configure the build
    CMAKE_ARGS=(
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"
        -Dunofficial-minizip_DIR="$MINIZIP_DIR"
        -DCMAKE_PREFIX_PATH="${BREW_PREFIX};${BREW_PREFIX}/opt/jpeg;${BREW_PREFIX}/opt/openal-soft;/usr/local"
        -DCMAKE_FIND_FRAMEWORK=LAST
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    )
    
    # Add bundle option if requested
    if [ "$BUNDLE_SDL2" = true ]; then
        CMAKE_ARGS+=(-DBUNDLE_SDL2=ON)
    fi
    
    # Add static build option if requested
    if [ "$BUILD_STATIC" = true ]; then
        CMAKE_ARGS+=(-DBUILD_STATIC=ON)
    fi
    
    # Use explicit CMake path to ensure correct version (CMake 3.24+ required)
    CMAKE_BIN="/opt/homebrew/bin/cmake"
    if [ ! -x "$CMAKE_BIN" ]; then
        CMAKE_BIN="cmake"  # fallback to system cmake
    fi
    
    if [ "$NINJA_AVAILABLE" = true ]; then
        "$CMAKE_BIN" .. -G Ninja "${CMAKE_ARGS[@]}"
    else
        "$CMAKE_BIN" .. "${CMAKE_ARGS[@]}"
    fi
else
    echo "CMake configuration already exists, skipping configuration step"
fi

# Build the application
if [ "$NINJA_AVAILABLE" = true ] && [ -f "$BUILD_DIR/build.ninja" ]; then
    ninja
else
    make -j$MAKE_JOBS
fi

echo "Build complete!"
echo ""
if [ "$BUNDLE_SDL2" = true ]; then
    echo "✅ Bundled build complete! The binary includes SDL2 and is portable to other macOS systems."
    echo ""
    echo "You can run the dynamix with:"
    echo "cd build && ./dynamix"
    echo ""
    echo "The binary can be distributed to other macOS 10.15+ systems without requiring SDL2 installation."
    echo "System frameworks (OpenAL, OpenGL, etc.) are still required but are always available on macOS."
else
    echo "You can run the dynamix with:"
    echo "cd build && ./dynamix"
    echo ""
    echo "Note: This build requires SDL2 to be installed on the target system."
fi
echo ""
echo "To clean and rebuild, run:"
echo "./build-macos.sh clean"
echo ""
echo "To build in debug mode, run:"
echo "./build-macos.sh debug"
echo ""
echo "To build a bundled binary, run:"
echo "./build-macos.sh bundle" 