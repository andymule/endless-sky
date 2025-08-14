#!/bin/bash
set -e

echo "=== Dynamix Fresh Install Setup ==="
echo "This script will install all required dependencies for building Dynamix on macOS"
echo

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check if Homebrew package is installed
homebrew_package_installed() {
    brew list --formula | grep -q "^$1\$"
}

# Get the script directory
SCRIPT_DIR="$(dirname "$0")"
cd "$SCRIPT_DIR"
DYNAMIX_DIR=$(pwd)
echo "Dynamix directory: $DYNAMIX_DIR"

echo "1. Checking for Homebrew..."
if ! command_exists brew; then
    echo "   Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    
    # Add Homebrew to PATH for current session
    if [[ $(uname -m) == "arm64" ]]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    else
        eval "$(/usr/local/bin/brew shellenv)"
    fi
else
    echo "   ✅ Homebrew already installed"
fi

echo "2. Checking for Xcode Command Line Tools..."
if ! xcode-select -p &> /dev/null; then
    echo "   Installing Xcode Command Line Tools..."
    xcode-select --install
    echo "   ⚠️  Please complete the Xcode Command Line Tools installation and run this script again"
    exit 1
else
    echo "   ✅ Xcode Command Line Tools already installed"
fi

echo "3. Installing required Homebrew packages..."
REQUIRED_PACKAGES=("cmake" "ninja" "pkg-config" "sdl2" "libpng" "jpeg" "openal-soft")
MISSING_PACKAGES=()

for package in "${REQUIRED_PACKAGES[@]}"; do
    if homebrew_package_installed "$package"; then
        echo "   ✅ $package already installed"
    else
        MISSING_PACKAGES+=("$package")
    fi
done

if [ ${#MISSING_PACKAGES[@]} -gt 0 ]; then
    echo "   Installing missing packages: ${MISSING_PACKAGES[*]}"
    brew install "${MISSING_PACKAGES[@]}"
    echo "   ✅ All packages installed"
else
    echo "   ✅ All required packages already installed"
fi

echo "4. Checking Git..."
if ! command_exists git; then
    echo "   ❌ Git not found. Please install Git and run this script again."
    exit 1
else
    echo "   ✅ Git available"
fi

echo "5. Checking vcpkg setup..."
PARENT_DIR="$(dirname "$DYNAMIX_DIR")"
if [ -d "$PARENT_DIR/vcpkg" ]; then
    echo "   ✅ Parent project vcpkg found at: $PARENT_DIR/vcpkg"
    
    if [ ! -f "$PARENT_DIR/vcpkg/vcpkg" ]; then
        echo "   Bootstrapping parent vcpkg..."
        cd "$PARENT_DIR/vcpkg"
        ./bootstrap-vcpkg.sh -disableMetrics
        cd "$DYNAMIX_DIR"
        echo "   ✅ Parent vcpkg bootstrapped"
    else
        echo "   ✅ Parent vcpkg already bootstrapped"
    fi
else
    echo "   No parent vcpkg found. Will create local vcpkg when needed."
fi

echo "6. Testing build..."
echo "   Running a test build to ensure everything works..."
if ./build-macos.sh debug; then
    echo "   ✅ Test build successful!"
else
    echo "   ❌ Test build failed. Please check the error messages above."
    exit 1
fi

echo
echo "🎉 Setup complete! Your Dynamix development environment is ready."
echo
echo "📋 Next Steps:"
echo "  1. Install the CodeLLDB VS Code extension for debugging support:"
echo "     - Open VS Code"
echo "     - Go to Extensions (Cmd+Shift+X)"
echo "     - Search for 'CodeLLDB' by vadimcn"
echo "     - Install it"
echo
echo "  2. You can now use:"
echo "     • The 'Build and Run (Fast - macOS/Linux)' launch configuration in VS Code"
echo "     • Run './build-macos.sh debug' for debug builds"
echo "     • Run './build-macos.sh bundle' for distributable builds"
echo
echo "All dependencies are now installed and configured."
