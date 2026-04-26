#!/bin/bash
set -e

# plainmd release build script
# Builds a release binary using the system Qt6 installation.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

# ---------------------------------------------------------------------------
# Locate system qmake (avoid local Qt installations)
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

echo "Building release with: $SYS_QMAKE"

# ---------------------------------------------------------------------------
# Generate Makefile and build
# ---------------------------------------------------------------------------
$SYS_QMAKE plainmd.pro CONFIG+=release
make -f Makefile.Release -j$(nproc)

# ---------------------------------------------------------------------------
# Verify output
# ---------------------------------------------------------------------------
if [ ! -x "release/plainmd" ]; then
    echo "Error: Build failed — release/plainmd not found."
    exit 1
fi

echo ""
echo "Success: release/plainmd"
echo ""
echo "Run with:  ./release/plainmd"
