#!/bin/bash
set -e

# plainmd clean script
# Removes all build artifacts to return the repo to a clean state.

cd "$(dirname "${BASH_SOURCE[0]}")"

echo "Cleaning build artifacts..."

# Build directories
rm -rf release/
rm -rf debug/

# Makefiles and qmake stash
rm -f Makefile
rm -f Makefile.Debug
rm -f Makefile.Release
rm -f .qmake.stash

# Qt generated files (fallback for root-level debris)
rm -f *.o
rm -f moc_*.cpp
rm -f moc_*.h
rm -f ui_*.h
rm -f qrc_*.cpp

# Backup and core files
rm -f *~
rm -f core
rm -f *.core

# Flatpak build artifacts
rm -rf build-dir/
rm -rf repo/
rm -rf .flatpak-builder/

echo "Done."
