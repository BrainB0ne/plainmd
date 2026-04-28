#!/bin/bash
set -e

# plainmd Debian package builder
# Builds a .deb from the release binary, linked against system Qt6.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# ---------------------------------------------------------------------------
# 1. Locate system qmake (avoid local Qt installations)
# ---------------------------------------------------------------------------
QMAKE=""
for candidate in /usr/bin/qmake6 /usr/lib/qt6/bin/qmake /usr/bin/qmake; do
    if [ -x "$candidate" ]; then
        ver="$($candidate --version 2>/dev/null | grep -oP 'Qt version \K[0-9]+\.[0-9]+' || true)"
        if [ -n "$ver" ] && [ -z "$(echo "$candidate" | grep -E 'Qt-[0-9]|/Qt/[0-9]')" ]; then
            QMAKE="$candidate"
            break
        fi
    fi
done

if [ -z "$QMAKE" ]; then
    echo "Error: Could not find system Qt6 qmake."
    echo "Install qt6-base-dev (or equivalent) and try again."
    exit 1
fi

echo "Using qmake: $QMAKE  (Qt $ver)"

# ---------------------------------------------------------------------------
# 2. Build release binary against system Qt
# ---------------------------------------------------------------------------
echo "Building release binary..."
$QMAKE plainmd.pro CONFIG+=release
make -f Makefile.Release

if [ ! -x "release/plainmd" ]; then
    echo "Error: release/plainmd was not built."
    exit 1
fi

# Verify it links to system libs, not a local Qt installation
if ldd release/plainmd | grep -qE '/home/.*/Qt/[0-9]|/usr/local/Qt'; then
    echo "Warning: Binary still links to a local Qt installation."
    echo "The resulting .deb will NOT work on other machines."
    echo "Install qt6-base-dev and rebuild against system Qt."
    exit 1
fi

echo "Binary links to system Qt — OK."

# ---------------------------------------------------------------------------
# 3. Package metadata
# ---------------------------------------------------------------------------
PKG_NAME="plainmd"
PKG_VERSION="1.3.0"
PKG_ARCH="amd64"
BUILD_DIR="build-deb"
PKG_DIR="${BUILD_DIR}/${PKG_NAME}_${PKG_VERSION}_${PKG_ARCH}"

# ---------------------------------------------------------------------------
# 4. Assemble Debian package tree
# ---------------------------------------------------------------------------
echo "Assembling package..."
rm -rf "${BUILD_DIR}"
mkdir -p "${PKG_DIR}/DEBIAN"
mkdir -p "${PKG_DIR}/usr/bin"
mkdir -p "${PKG_DIR}/usr/share/applications"
mkdir -p "${PKG_DIR}/usr/share/pixmaps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/256x256/apps"

# Binary
cp release/plainmd "${PKG_DIR}/usr/bin/"
chmod 755 "${PKG_DIR}/usr/bin/plainmd"

# .desktop entry
cat > "${PKG_DIR}/usr/share/applications/plainmd.desktop" << 'EOF'
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
chmod 644 "${PKG_DIR}/usr/share/applications/plainmd.desktop"

# Icon — hicolor theme path (used by GTK/Cinnamon/GNOME start menus)
cp icon.png "${PKG_DIR}/usr/share/icons/hicolor/256x256/apps/plainmd.png"
chmod 644 "${PKG_DIR}/usr/share/icons/hicolor/256x256/apps/plainmd.png"

# Fallback for Qt-only / older systems
cp icon.png "${PKG_DIR}/usr/share/pixmaps/plainmd.png"
chmod 644 "${PKG_DIR}/usr/share/pixmaps/plainmd.png"

# ---------------------------------------------------------------------------
# 5. DEBIAN/control
# ---------------------------------------------------------------------------
# Qt6 package names vary by distro:
#   Debian 13 / Ubuntu 24.04+ : libqt6core6t64, libqt6gui6t64, ...
#   Older releases              : libqt6core6,   libqt6gui6,   ...
# Adjust the Depends line below for your target distribution.
cat > "${PKG_DIR}/DEBIAN/control" << EOF
Package: ${PKG_NAME}
Version: ${PKG_VERSION}
Section: utils
Priority: optional
Architecture: ${PKG_ARCH}
Depends: libqt6core6t64 | libqt6core6, libqt6gui6t64 | libqt6gui6, libqt6widgets6t64 | libqt6widgets6, libqt6network6t64 | libqt6network6, libqt6printsupport6t64 | libqt6printsupport6
Maintainer: PlainMD Team
Description: A simple and elegant Markdown viewer
 PlainMD is a lightweight desktop Markdown viewer built with Qt6.
 It supports .md, .markdown, .mdx, and .txt files with a built-in
 file browser, external image preview, and print support.
EOF

# ---------------------------------------------------------------------------
# 6. Build .deb
# ---------------------------------------------------------------------------
echo "Building .deb package..."
dpkg-deb --build "${PKG_DIR}"

# Create dist folder
mkdir -p dist

mv "${BUILD_DIR}/${PKG_NAME}_${PKG_VERSION}_${PKG_ARCH}.deb" dist/

# Clean up build tree
rm -rf "${BUILD_DIR}"

echo ""
echo "Success: dist/${PKG_NAME}_${PKG_VERSION}_${PKG_ARCH}.deb"
echo ""
echo "Install with:  sudo dpkg -i dist/${PKG_NAME}_${PKG_VERSION}_${PKG_ARCH}.deb"
echo "Or:            sudo apt install ./dist/${PKG_NAME}_${PKG_VERSION}_${PKG_ARCH}.deb"
