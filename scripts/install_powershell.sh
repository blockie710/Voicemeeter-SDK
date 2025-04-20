#!/bin/bash

# Create a temp directory for the download
mkdir -p /tmp/ps-install

# Download the PowerShell package
echo "Downloading PowerShell..."
curl -L -o /tmp/ps-install/powershell.tar.gz https://github.com/PowerShell/PowerShell/releases/download/v7.4.1/powershell-7.4.1-linux-x64.tar.gz

# Create the target directory
sudo mkdir -p /opt/microsoft/powershell/7

# Extract PowerShell to the target directory
echo "Installing PowerShell..."
sudo tar zxf /tmp/ps-install/powershell.tar.gz -C /opt/microsoft/powershell/7

# Create symbolic link to make PowerShell accessible
sudo chmod +x /opt/microsoft/powershell/7/pwsh
sudo ln -sf /opt/microsoft/powershell/7/pwsh /usr/bin/pwsh

# Clean up
rm -rf /tmp/ps-install

echo "PowerShell has been successfully installed."
echo "You can start PowerShell by running 'pwsh'"
