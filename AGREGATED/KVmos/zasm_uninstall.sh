#!/bin/bash

echo -e "\033[1;35m[ZASM Uninstaller]\033[0m"

if [[ $EUID -ne 0 ]]; then
  echo -e "\033[1;31m[ERROR]\033[0m Please run as root. Try: sudo ./zasm_uninstall.sh"
  exit 1
fi

rm -f /usr/local/bin/zasm
rm -rf /usr/local/share/zasm

echo -e "\033[1;32m[SUCCESS]\033[0m ZASM has been removed."
