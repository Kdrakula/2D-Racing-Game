#!/bin/bash
# Updater script for MyRacingGame on macOS
# Usage: ./updater.sh <DOWNLOAD_URL> <APP_PATH>

DOWNLOAD_URL=$1
APP_PATH=$2
TEMP_ZIP="/tmp/MyRacingGameUpdate.zip"
TEMP_DIR="/tmp/MyRacingGameUpdateExtract"

echo "Starting Update Process..."
echo "Downloading from: $DOWNLOAD_URL"
echo "Target App Path: $APP_PATH"

# Sleep briefly to ensure the parent C++ process (which launched this) has fully closed.
sleep 2

# Download the new zip
curl -L -o "$TEMP_ZIP" "$DOWNLOAD_URL"

if [ $? -ne 0 ]; then
    echo "Download failed!"
    exit 1
fi

# Prepare extraction directory
rm -rf "$TEMP_DIR"
mkdir -p "$TEMP_DIR"

# Extract the zip
unzip -q "$TEMP_ZIP" -d "$TEMP_DIR"

if [ $? -ne 0 ]; then
    echo "Extraction failed!"
    exit 1
fi

# Replace the old App Bundle
echo "Replacing old bundle..."
rm -rf "$APP_PATH"
cp -R "$TEMP_DIR/MyRacingGame.app" "$APP_PATH"

# Cleanup
rm -f "$TEMP_ZIP"
rm -rf "$TEMP_DIR"

# Relaunch the App
echo "Relaunching game..."
open "$APP_PATH"

echo "Update complete."
exit 0
