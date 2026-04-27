#!/bin/bash
set -e

# Generate SHA256 checksums for distribution packages
# Creates individual .sha256 files for each file in dist/

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

if [ ! -d "dist" ]; then
    echo "Error: dist/ folder not found."
    echo "Build distribution packages first with:"
    echo "  ./build-deb.sh          # for .deb"
    echo "  ./build-appimage.sh     # for .AppImage"
    echo "  ./build-installer.bat   # for Windows .exe (run on Windows)"
    exit 1
fi

echo "Generating SHA256 checksums for distribution packages..."

cd dist

# Remove old checksum files
rm -f *.sha256 SHA256SUMS

# Generate individual .sha256 files for each package
for file in *; do
    # Skip directories and existing checksum files
    if [ -f "$file" ] && [[ ! "$file" =~ \.sha256$ ]]; then
        sha256sum "$file" > "${file}.sha256"
        echo "  ${file}.sha256"
    fi
done

# Also create a combined SHA256SUMS file
sha256sum * > SHA256SUMS 2>/dev/null || true

echo ""
echo "Checksum files created in dist/:"
ls -la *.sha256 SHA256SUMS 2>/dev/null || ls -la *.sha256
echo ""
echo "Verify a package with:"
echo "  sha256sum -c dist/<package>.sha256"
echo ""
echo "Or verify all with:"
echo "  cd dist && sha256sum -c SHA256SUMS"
