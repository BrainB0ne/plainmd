#!/bin/bash
set -e

# PlainMD Linux Release Pipeline
# Builds all distribution packages, generates checksums, and archives them.
# Output: dist/plainmd-<version>-release.zip (or .tar.gz)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

VERSION=$(grep -oP 'setApplicationVersion\("\K[0-9]+\.[0-9]+\.[0-9]+' src/main.cpp 2>/dev/null || echo "")
if [ -z "$VERSION" ]; then
    echo "Error: Could not extract version from src/main.cpp"
    exit 1
fi

echo "=========================================="
echo "PlainMD Release Build"
echo "Version: ${VERSION}"
echo "=========================================="
echo ""

# ---------------------------------------------------------------------------
# 1. Clean
# ---------------------------------------------------------------------------
echo "Step 1: Cleaning previous builds..."
./clean.sh
echo ""

# ---------------------------------------------------------------------------
# 2. Build .deb package
# ---------------------------------------------------------------------------
if command -v dpkg-deb >/dev/null 2>&1; then
    echo "Step 2: Building .deb package..."
    ./build-deb.sh
else
    echo "Step 2: SKIPPED — dpkg-deb not found (install with: sudo apt install dpkg-dev)"
fi
echo ""

# ---------------------------------------------------------------------------
# 3. Build AppImage
# ---------------------------------------------------------------------------
if [ -f ".appimage-tools/linuxdeploy-x86_64.AppImage" ] || command -v wget >/dev/null 2>&1 || command -v curl >/dev/null 2>&1; then
    echo "Step 3: Building AppImage..."
    ./build-appimage.sh
else
    echo "Step 3: SKIPPED — wget/curl not found (needed to download linuxdeploy tools)"
fi
echo ""

# ---------------------------------------------------------------------------
# 4. Build Flatpak
# ---------------------------------------------------------------------------
if command -v flatpak-builder >/dev/null 2>&1; then
    echo "Step 4: Building Flatpak..."
    ./build-flatpak.sh
else
    echo "Step 4: SKIPPED — flatpak-builder not found (install with: sudo apt install flatpak-builder)"
fi
echo ""

# ---------------------------------------------------------------------------
# 5. Generate checksums
# ---------------------------------------------------------------------------
if [ -d "dist" ] && [ -n "$(ls -A dist 2>/dev/null)" ]; then
    echo "Step 5: Generating checksums..."
    ./make-checksums.sh
else
    echo "Step 5: SKIPPED — no distribution packages were built."
fi
echo ""

# ---------------------------------------------------------------------------
# 6. Create release archive
# ---------------------------------------------------------------------------
if [ -d "dist" ] && [ -n "$(ls -A dist 2>/dev/null)" ]; then
    echo "Step 6: Creating release archive..."
    ./archive-release.sh
else
    echo "Step 6: SKIPPED — nothing to archive."
fi

echo ""
echo "=========================================="
echo "Release build complete!"
echo "=========================================="
if [ -d "dist" ]; then
    echo ""
    ls -lh dist/
fi
