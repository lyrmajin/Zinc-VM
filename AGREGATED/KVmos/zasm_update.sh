#!/bin/bash

# === CONFIG ===
BIN_NAME="zasm"
INSTALL_PATH="/usr/local/bin/$BIN_NAME"
VERSION_FILE_URL="https://raw.githubusercontent.com/LyraStudios-mk/Zasm/main/latest_version.txt"
RELEASE_URL_BASE="https://github.com/LyraStudios-mk/Zasm/releases/download"

# === DETECT PLATFORM ===
ARCH=$(uname -m)
OS=$(uname -s | tr '[:upper:]' '[:lower:]')

case "$ARCH" in
  x86_64) ARCH="x86_64" ;;
  aarch64) ARCH="arm64" ;;
  armv7l) ARCH="armv7" ;;
  *) echo -e "\033[1;31m[ERROR]\033[0m Unsupported architecture: $ARCH"; exit 1 ;;
esac

# === GET CURRENT VERSION ===
if [ -f "$INSTALL_PATH" ]; then
    RAW_VERSION=$("$INSTALL_PATH" --version 2>/dev/null | awk '{print $NF}')
    CURRENT_VERSION=$(echo "$RAW_VERSION" | sed 's/^ZASM://')
    if [[ -z "$CURRENT_VERSION" ]]; then
        echo -e "\033[1;33m[WARN]\033[0m Couldn't detect installed version. Assuming none."
        CURRENT_VERSION="0.0.0"
    fi
else
    echo -e "\033[1;33m[INFO]\033[0m ZASM not found at $INSTALL_PATH, assuming not installed."
    CURRENT_VERSION="0.0.0"
fi

echo -e "\033[1;36m[ZASM Updater]\033[0m Current version: $CURRENT_VERSION"

# === FETCH LATEST VERSION ===
RAW_LATEST_VERSION=$(curl -s "$VERSION_FILE_URL" | tr -d '\r\n')
echo "DEBUG: RAW_LATEST_VERSION = '$RAW_LATEST_VERSION'"

# Clean it
LATEST_VERSION=$(echo "$RAW_LATEST_VERSION" | sed 's/^ZASM://;s/^v//')
echo "DEBUG: Parsed LATEST_VERSION = '$LATEST_VERSION'"

if [[ -z "$LATEST_VERSION" ]]; then
  echo -e "\033[1;31m[ERROR]\033[0m Could not retrieve latest version info."
  exit 1
fi

echo "Latest version available: v$LATEST_VERSION"

# === COMPARE VERSIONS ===
if [[ "$LATEST_VERSION" == "$CURRENT_VERSION" ]]; then
  echo -e "\033[1;32m[OK]\033[0m Already up to date."
  exit 0
fi

# === DOWNLOAD AND INSTALL ===
FILENAME="zasm-v${LATEST_VERSION}-${OS}-${ARCH}"
TEMPFILE="/tmp/$FILENAME"
DOWNLOAD_URL="${RELEASE_URL_BASE}/v${LATEST_VERSION}/${FILENAME}"

echo "Downloading $FILENAME from $DOWNLOAD_URL..."
curl -L -o "$TEMPFILE" "$DOWNLOAD_URL"

if [[ ! -s "$TEMPFILE" ]] || [[ $(stat -c%s "$TEMPFILE") -lt 10000 ]]; then
  echo -e "\033[1;31m[ERROR]\033[0m Downloaded file is too small. Something went wrong."
  head "$TEMPFILE"
  exit 1
fi

chmod +x "$TEMPFILE"

# Backup
if [ -f "$INSTALL_PATH" ]; then
    sudo cp "$INSTALL_PATH" "${INSTALL_PATH}.bak"
    echo "Backup created at ${INSTALL_PATH}.bak"
fi

sudo mv "$TEMPFILE" "$INSTALL_PATH"

# Final confirmation
NEW_VERSION=$($BIN_NAME --version 2>/dev/null | awk '{print $NF}')
if [[ -z "$NEW_VERSION" ]]; then
  echo -e "\033[1;33m[WARN]\033[0m Could not verify updated version."
else
  echo -e "\033[1;32m[SUCCESS]\033[0m Updated ZASM to version $NEW_VERSION"
fi
