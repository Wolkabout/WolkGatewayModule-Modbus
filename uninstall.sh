#!/bin/bash

echo "Uninstalling WolkGatewayModule-Modbus..."
rm /etc/systemd/system/modbus_module.service
echo "Removed service unit file"
systemctl daemon-reload
echo "Systemctl reloaded"
rm -rf /etc/modbusModule/
echo "Removed configuration files"
rm -rf /usr/local/bin/lib/
rm -rf /usr/local/bin/modbusModule
echo "Removed application files"
echo "Done."
