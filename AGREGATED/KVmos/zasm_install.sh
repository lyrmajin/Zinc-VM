#!/bin/bash

# === CONFIG ===
BIN_NAME="zasm"
INSTALL_BIN="/usr/local/bin/$BIN_NAME"
SHARE_DIR="/usr/local/share/zasm"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo -e "\033[1;35m[ZASM Installer]\033[0m Starting installation..."

# Check root
if [[ $EUID -ne 0 ]]; then
  echo -e "\033[1;31m[ERROR]\033[0m This script must be run as root. Try: sudo ./zasm_install.sh"
  exit 1
fi

# Install binary
echo -n "Installing binary to $INSTALL_BIN... "
install -m 755 "$SCRIPT_DIR/$BIN_NAME" "$INSTALL_BIN" && echo "done." || {
  echo -e "\n\033[1;31m[ERROR]\033[0m Failed to copy binary."
  exit 1
}

# Install stdlib files
echo -n "Installing standard library to $SHARE_DIR... "
mkdir -p "$SHARE_DIR" || {
  echo -e "\n\033[1;31m[ERROR]\033[0m Failed to create directory."
  exit 1
}
cp -r "$SCRIPT_DIR/stdzio" "$SHARE_DIR/" && echo "done." || {
  echo -e "\n\033[1;31m[ERROR]\033[0m Failed to copy stdzio."
  exit 1
}

echo -e "\033[1;32m[SUCCESS]\033[0m ZASM has been installed to \033[1;36m$INSTALL_BIN\033[0m"
echo -e "Standard library installed to \033[1;36m$SHARE_DIR/stdzio\033[0m"
