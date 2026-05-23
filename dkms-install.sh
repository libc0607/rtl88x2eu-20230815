#!/bin/bash

if [[ $EUID -ne 0 ]]; then
  echo "You must run this with superuser privileges. Try \"sudo ./dkms-install.sh\"" 2>&1
  exit 1
fi

DRV_NAME=rtl88x2eu
DRV_VERSION=5.15.0.1

echo "About to run dkms install steps..."
cp -r "$(pwd)" /usr/src/${DRV_NAME}-${DRV_VERSION}

dkms add -m ${DRV_NAME} -v ${DRV_VERSION}
dkms build -m ${DRV_NAME} -v ${DRV_VERSION}
dkms install -m ${DRV_NAME} -v ${DRV_VERSION}
RESULT=$?

if [ $RESULT -eq 0 ]; then
    echo "Driver installation successful. Configuring network tweaks..."
    
    # 1. Load the module so the interface appears
    modprobe $DRV_NAME

    # 2. Identify the specific interface name for this driver
    # This looks for the interface associated with the rtl8812au driver
    WIFI_IFACE=$(basename $(ls -l /sys/class/net/*/device/driver | grep "$DRV_NAME" | awk '{print $9}' | cut -d/ -f5) 2>/dev/null)

    if [ -n "$WIFI_IFACE" ]; then
        echo "Found interface $WIFI_IFACE. Disabling IPv6 for this device only..."
        
        # Apply change immediately
        sysctl -w net.ipv6.conf."$WIFI_IFACE".disable_ipv6=1
        
        # Make it permanent by adding to a specific config file instead of cluttering sysctl.conf
        CONF_FILE="/etc/sysctl.d/99-${DRV_NAME}.conf"
        echo "net.ipv6.conf.$WIFI_IFACE.disable_ipv6 = 1" > "$CONF_FILE"
        echo "Configuration written to $CONF_FILE"
    else
        echo "Could not auto-detect interface name. No global IPv6 changes made."
    fi
else
    echo "DKMS install failed."
fi

exit $RESULT