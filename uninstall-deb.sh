#!/bin/bash
set -e

# plainmd uninstall script
# Removes the Debian package via apt/dpkg and optionally purges user data.

if [ "$EUID" -ne 0 ]; then
    echo "This script must be run as root (use sudo)."
    exit 1
fi

if ! dpkg -s plainmd >/dev/null 2>&1; then
    echo "Package 'plainmd' is not installed."
    exit 0
fi

echo "Removing plainmd via package manager..."
if command -v apt >/dev/null 2>&1; then
    apt remove --purge -y plainmd
else
    dpkg -r plainmd
fi

# Refresh icon cache so the start menu updates immediately
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache /usr/share/icons/hicolor || true
fi

echo "plainmd has been removed from the system."

# ---------------------------------------------------------------------------
# Optionally purge user data
# ---------------------------------------------------------------------------
read -r -p "Also delete user settings and cached external images? [y/N] " response
if [[ "$response" =~ ^[Yy]$ ]]; then
    # Default QSettings path: ~/.config/plainmd/PlainMD.ini
    rm -rf ~/.config/plainmd
    # External image cache: /tmp/plainmd_images
    rm -rf /tmp/plainmd_images
    echo "User data purged."
else
    echo "User data kept."
fi

echo "Done."
