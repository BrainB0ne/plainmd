#!/bin/bash
set -e

# plainmd install script
# Installs the local .deb package and resolves dependencies automatically.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ "$EUID" -ne 0 ]; then
    echo "This script must be run as root (use sudo)."
    exit 1
fi

# Find the .deb package in dist/
DEB_FILE=$(ls -t "${SCRIPT_DIR}/dist/plainmd_"*"_amd64.deb" 2>/dev/null | head -n 1)

if [ -z "$DEB_FILE" ] || [ ! -f "$DEB_FILE" ]; then
    echo "Error: .deb package not found in dist/."
    echo "Run ./build-deb.sh first to build the .deb package."
    exit 1
fi

echo "Installing: $(basename "$DEB_FILE")"

# apt's sandboxed _apt user cannot read files in user home directories.
# Copy to /tmp first to avoid the "unsandboxed as root" warning.
TMP_DEB="/tmp/plainmd_install.deb"
cp "$DEB_FILE" "$TMP_DEB"

# apt install resolves dependencies automatically; dpkg -i does not.
if command -v apt >/dev/null 2>&1; then
    apt install -y "$TMP_DEB"
else
    dpkg -i "$TMP_DEB" || true
    # Attempt to fix missing dependencies if dpkg failed
    if command -v apt-get >/dev/null 2>&1; then
        apt-get install -f -y
    fi
fi

rm -f "$TMP_DEB"

# Refresh icon cache so the Start Menu picks up the icon immediately
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache /usr/share/icons/hicolor || true
fi

echo ""
echo "plainmd installed successfully."
echo "Launch from the Start Menu or run:  plainmd"
