#!/bin/bash
set -e

# Uninstall PlainMD Flatpak from the system

APP_ID="eu.brainbytez.plainmd"

echo "=========================================="
echo "Uninstalling PlainMD Flatpak"
echo "=========================================="
echo "App ID: ${APP_ID}"
echo "=========================================="
echo ""

# Check if flatpak is installed
if ! command -v flatpak &> /dev/null; then
    echo "Error: flatpak is not installed"
    exit 1
fi

# Check if the app is installed
if ! flatpak list --user | grep -q "${APP_ID}"; then
    if ! flatpak list --system | grep -q "${APP_ID}"; then
        echo "PlainMD is not installed as a Flatpak."
        echo "Nothing to uninstall."
        exit 0
    fi
fi

echo "Found PlainMD installation."
echo ""

# Uninstall the app
echo "Uninstalling ${APP_ID}..."
flatpak uninstall --user -y "${APP_ID}" 2>/dev/null || \
flatpak uninstall --system -y "${APP_ID}" 2>/dev/null || true

# Also remove unused dependencies
echo ""
echo "Removing unused dependencies..."
flatpak uninstall --user -y --unused 2>/dev/null || true
flatpak uninstall --system -y --unused 2>/dev/null || true

echo ""
echo "=========================================="
echo "PlainMD Flatpak has been uninstalled!"
echo "=========================================="
echo ""
echo "To reinstall:"
echo "  ./build-flatpak.sh"
echo "  flatpak install dist/plainmd-*.flatpak"
echo "=========================================="
