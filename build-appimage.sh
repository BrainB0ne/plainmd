#!/bin/bash
set -e

# plainmd AppImage builder
# Bundles the release binary + Qt6 libraries into a single portable AppImage.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# Package version
PKG_VERSION="1.2.0"

# ---------------------------------------------------------------------------
# 1. Locate system qmake and build against it
# ---------------------------------------------------------------------------
SYS_QMAKE=""
for candidate in /usr/bin/qmake6 /usr/lib/qt6/bin/qmake /usr/bin/qmake; do
    if [ -x "$candidate" ] && [ -z "$(echo "$candidate" | grep -E 'Qt-[0-9]|/Qt/[0-9]')" ]; then
        ver="$($candidate --version 2>/dev/null | grep -oP 'Qt version \K[0-9]+\.[0-9]+' || true)"
        if [ -n "$ver" ]; then
            SYS_QMAKE="$candidate"
            break
        fi
    fi
done

if [ -z "$SYS_QMAKE" ]; then
    echo "Error: Could not find system Qt6 qmake."
    echo "Install qt6-base-dev (or equivalent) and try again."
    exit 1
fi

echo "Building release binary with system Qt ($SYS_QMAKE)..."
$SYS_QMAKE plainmd.pro CONFIG+=release
make -f Makefile.Release

if [ ! -x "release/plainmd" ]; then
    echo "Error: release/plainmd was not built."
    exit 1
fi

# Verify it links to system libs, not a local Qt installation
if ldd release/plainmd | grep -qE '/home/.*/Qt/[0-9]|/usr/local/Qt'; then
    echo "Warning: Binary still links to a local Qt installation."
    echo "The resulting AppImage will NOT be portable."
    exit 1
fi

echo "Binary links to system Qt — OK."

# ---------------------------------------------------------------------------
# 2. Download linuxdeploy tools if missing
# ---------------------------------------------------------------------------
TOOLS_DIR="${SCRIPT_DIR}/.appimage-tools"
mkdir -p "${TOOLS_DIR}"

LINUXDEPLOY="${TOOLS_DIR}/linuxdeploy-x86_64.AppImage"
LINUXDEPLOY_QT="${TOOLS_DIR}/linuxdeploy-plugin-qt-x86_64.AppImage"
APPIMAGETOOL="${TOOLS_DIR}/appimagetool-x86_64.AppImage"

download_if_missing() {
    local url="$1"
    local dest="$2"
    if [ ! -f "$dest" ]; then
        echo "Downloading $(basename "$dest")..."
        wget -q --show-progress "$url" -O "$dest" || curl -sL "$url" -o "$dest"
        chmod +x "$dest"
    fi
}

download_if_missing \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    "$LINUXDEPLOY"

download_if_missing \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
    "$LINUXDEPLOY_QT"

download_if_missing \
    "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" \
    "$APPIMAGETOOL"

# Helper to run AppImages even when FUSE is unavailable
run_appimage() {
    local app="$1"
    shift
    if "$app" "$@" 2>/dev/null; then
        return 0
    else
        # FUSE may be missing; extract and run
        local tmpdir="/tmp/appimage-run-$$"
        mkdir -p "$tmpdir"
        "$app" --appimage-extract >/dev/null 2>&1
        local extracted="squashfs-root"
        if [ -d "$extracted" ]; then
            "$extracted/AppRun" "$@"
            local rc=$?
            rm -rf "$extracted"
            return $rc
        fi
        return 1
    fi
}

# ---------------------------------------------------------------------------
# 3. Prepare AppDir
# ---------------------------------------------------------------------------
echo "Preparing AppDir..."
rm -rf AppDir
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons/hicolor/256x256/apps

# Binary
cp release/plainmd AppDir/usr/bin/
chmod 755 AppDir/usr/bin/plainmd

# Desktop entry
cat > AppDir/usr/share/applications/plainmd.desktop << 'EOF'
[Desktop Entry]
Name=PlainMD
GenericName=Markdown Viewer
Comment=A simple and elegant Markdown viewer
Exec=plainmd %F
Icon=plainmd
Type=Application
Categories=Office;Viewer;
MimeType=text/markdown;text/x-markdown;text/plain;
Terminal=false
StartupNotify=true
EOF
chmod 644 AppDir/usr/share/applications/plainmd.desktop

# Icon
cp icon.png AppDir/usr/share/icons/hicolor/256x256/apps/plainmd.png
chmod 644 AppDir/usr/share/icons/hicolor/256x256/apps/plainmd.png

# linuxdeploy/appimagetool convention: place these in AppDir root too
cp AppDir/usr/share/applications/plainmd.desktop AppDir/plainmd.desktop
cp AppDir/usr/share/icons/hicolor/256x256/apps/plainmd.png AppDir/plainmd.png

# ---------------------------------------------------------------------------
# 4. Bundle Qt6 libraries and plugins with linuxdeploy
# ---------------------------------------------------------------------------
echo "Bundling Qt6 dependencies..."
export LDAI_PLUGIN_QT="${LINUXDEPLOY_QT}"
export QMAKE="${SYS_QMAKE}"   # force linuxdeploy-plugin-qt to use system Qt
run_appimage "$LINUXDEPLOY" \
    --appdir "${SCRIPT_DIR}/AppDir" \
    --desktop-file "${SCRIPT_DIR}/AppDir/plainmd.desktop" \
    --icon-file "${SCRIPT_DIR}/AppDir/plainmd.png" \
    --plugin qt

# ---------------------------------------------------------------------------
# 5. Create AppImage
# ---------------------------------------------------------------------------
echo "Creating AppImage..."
mkdir -p dist
run_appimage "$APPIMAGETOOL" \
    "${SCRIPT_DIR}/AppDir" \
    "${SCRIPT_DIR}/dist/plainmd-${PKG_VERSION}-x86_64.AppImage"

# ---------------------------------------------------------------------------
# 6. Cleanup
# ---------------------------------------------------------------------------
rm -rf AppDir

echo ""
echo "Success: dist/plainmd-${PKG_VERSION}-x86_64.AppImage"
echo ""
echo "Run with:  ./dist/plainmd-${PKG_VERSION}-x86_64.AppImage"
echo "Or make executable and double-click in your file manager."
