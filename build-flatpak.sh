#!/bin/bash
set -e

# Build Flatpak package for PlainMD
# Output: dist/plainmd-<version>.flatpak

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# Extract version from src/main.cpp
VERSION=$(grep -oP 'setApplicationVersion\("\K[0-9]+\.[0-9]+\.[0-9]+' src/main.cpp 2>/dev/null || echo "")

if [ -z "$VERSION" ]; then
    echo "Warning: Could not extract version from src/main.cpp"
    VERSION="unknown"
fi

# Detect architecture
ARCH=$(uname -m)

OUTPUT_FILE="plainmd-${VERSION}-${ARCH}.flatpak"

echo "=========================================="
echo "Building Flatpak package"
echo "=========================================="
echo "Version: ${VERSION}"
echo "Arch:    ${ARCH}"
echo "App ID:  eu.brainbytez.plainmd"
echo "Output:  dist/${OUTPUT_FILE}"
echo "=========================================="
echo ""

# Check if flatpak-builder is installed
if ! command -v flatpak-builder &> /dev/null; then
    echo "Error: flatpak-builder is not installed"
    echo "Please install it with: sudo apt install flatpak-builder"
    exit 1
fi

# Check if KDE runtime is installed (user installation)
if ! flatpak list --user | grep -q "org.kde.Platform.*6.7"; then
    echo "KDE Platform 6.7 not found. Installing..."
    echo "This is a one-time setup and may take a while."
    echo ""
    flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
    flatpak install --user flathub org.kde.Platform//6.7 org.kde.Sdk//6.7 -y
fi

# Create dist directory if it doesn't exist
mkdir -p dist

# Clean previous build
if [ -d "build-dir" ]; then
    echo "Cleaning previous build..."
    rm -rf build-dir
fi

# Build the Flatpak
echo "Building Flatpak (this may take a few minutes)..."
flatpak-builder --repo=repo --force-clean build-dir eu.brainbytez.plainmd.yaml

# Create the bundle
echo "Creating distributable bundle..."
flatpak build-bundle repo "dist/${OUTPUT_FILE}" eu.brainbytez.plainmd

# Clean up build directory
rm -rf build-dir

echo ""
echo "=========================================="
echo "Flatpak package created successfully!"
echo "=========================================="
ls -lh "dist/${OUTPUT_FILE}"
echo ""
echo "Install with:"
echo "  flatpak install dist/${OUTPUT_FILE}"
echo ""
echo "Run with:"
echo "  flatpak run eu.brainbytez.plainmd"
echo "=========================================="
