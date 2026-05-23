#!/bin/bash

if [[ $EUID -ne 0 ]]; then
  echo "You must run this with superuser privileges."
  exit 1
fi

DRV_NAME=rtl88x2eu
DRV_VERSION=5.15.0.1
CONF_FILE="/etc/sysctl.d/99-${DRV_NAME}.conf"

echo "Removing $DRV_NAME $DRV_VERSION from DKMS..."

dkms remove -m ${DRV_NAME} -v ${DRV_VERSION} --all

# Clean up the specific networking tweak we added
if [ -f "$CONF_FILE" ]; then
    echo "Removing driver-specific networking tweaks..."
    rm "$CONF_FILE"
    # Apply changes to re-enable IPv6 for whatever interface was blocked
    sysctl --system
fi

echo "Finished removal."