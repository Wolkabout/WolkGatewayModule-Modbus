#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as sudo."
  exit
fi

echo "Uninstalling WolkGatewayModule-Modbus..."
service modbus_module stop
echo "Stopped running service"
rm /etc/systemd/system/modbus_module.service
echo "Removed service unit file"
systemctl daemon-reload
echo "Systemctl reloaded"
rm -rf /etc/modbusModule/
echo "Removed configuration files"
rm -rf /usr/local/bin/lib/
rm /usr/local/bin/modbusModule
echo "Removed application files"
echo "Done."
