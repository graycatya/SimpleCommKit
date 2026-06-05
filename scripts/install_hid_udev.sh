#!/bin/bash
# Install udev rules for HID device access
# Run with: sudo bash install_udev.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RULES_FILE="$SCRIPT_DIR/99-hidraw.rules"
TARGET="/etc/udev/rules.d/99-hidraw.rules"

if [ "$EUID" -ne 0 ]; then
    echo "Please run with sudo: sudo bash install_udev.sh"
    exit 1
fi

if [ ! -f "$RULES_FILE" ]; then
    echo "Error: $RULES_FILE not found"
    exit 1
fi

cp "$RULES_FILE" "$TARGET"
echo "Installed: $TARGET"

udevadm control --reload-rules
udevadm trigger
echo "Rules applied. /dev/hidraw* devices are now accessible by non-root users."
