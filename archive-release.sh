#!/bin/bash
set -e

# Archive the dist folder into a versioned zip file
# Output: dist/plainmd-<version>-release.zip

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# Check if dist folder exists
if [ ! -d "dist" ]; then
    echo "Error: dist/ folder not found."
    echo "Build distribution packages first with:"
    echo "  ./build-deb.sh          # for .deb"
    echo "  ./build-appimage.sh     # for .AppImage"
    echo "  ./build-installer.bat   # for Windows .exe (run on Windows)"
    exit 1
fi

# Check if dist folder has any files
if [ -z "$(ls -A dist 2>/dev/null)" ]; then
    echo "Error: dist/ folder is empty."
    echo "Please build distribution packages first."
    exit 1
fi

# Extract version from main.cpp
VERSION=$(grep -oP 'setApplicationVersion\("\K[0-9]+\.[0-9]+\.[0-9]+' src/main.cpp 2>/dev/null || echo "")

if [ -z "$VERSION" ]; then
    echo "Warning: Could not extract version from src/main.cpp"
    echo "Using 'unknown' as version."
    VERSION="unknown"
fi

OUTPUT_FILE="plainmd-${VERSION}-release.zip"

echo "=========================================="
echo "Creating release archive"
echo "=========================================="
echo "Version: ${VERSION}"
echo "Source:  dist/"
echo "Output:  dist/${OUTPUT_FILE}"
echo "=========================================="
echo ""

# Create zip archive inside dist folder
cd dist
# Remove old archive if exists
rm -f "${OUTPUT_FILE}"

# Create zip excluding the zip file itself
zip -r "${OUTPUT_FILE}" . -x "${OUTPUT_FILE}"

echo ""
echo "=========================================="
echo "Archive created successfully!"
echo "=========================================="
ls -lh "${OUTPUT_FILE}"
echo ""
echo "Archive contents:"
unzip -l "${OUTPUT_FILE}" | tail -4
