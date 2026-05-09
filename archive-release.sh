#!/bin/bash
set -e

# Archive the dist folder into a versioned zip file
# Output: dist/plainmd-<version>-linux-release.zip

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

OUTPUT_FILE="plainmd-${VERSION}-linux-release.zip"

echo "=========================================="
echo "Creating release archive"
echo "=========================================="
echo "Version: ${VERSION}"
echo "Source:  dist/"
echo "Output:  dist/${OUTPUT_FILE}"
echo "=========================================="
echo ""

# Create archive inside dist folder
cd dist
# Remove old archive if exists
rm -f "${OUTPUT_FILE}"

# Prefer zip, fall back to tar.gz if zip is unavailable
if command -v zip >/dev/null 2>&1; then
    zip -r "${OUTPUT_FILE}" . -x "${OUTPUT_FILE}"
else
    echo "'zip' not found, falling back to tar.gz..."
    TAR_FILE="${OUTPUT_FILE%.zip}.tar.gz"
    rm -f "${TAR_FILE}"
    tar -czf "${TAR_FILE}" --exclude="${OUTPUT_FILE}" --exclude="${TAR_FILE}" --transform='s,^\./,,' .
    OUTPUT_FILE="${TAR_FILE}"
fi

echo ""
echo "=========================================="
echo "Archive created successfully!"
echo "=========================================="
ls -lh "${OUTPUT_FILE}"
echo ""
echo "Archive contents:"
if [[ "${OUTPUT_FILE}" == *.zip ]] && command -v unzip >/dev/null 2>&1; then
    unzip -l "${OUTPUT_FILE}" | tail -4
else
    tar -tzf "${OUTPUT_FILE}" | head -10
    echo "..."
fi
