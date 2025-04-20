# Development environment setup script for Voicemeeter Plugin Host on Windows

Write-Host "Setting up development environment for Voicemeeter Plugin Host..." -ForegroundColor Cyan

# Check admin privileges
if (-NOT ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Warning "This script should be run as Administrator for package installations."
    Write-Host "Press Enter to continue anyway or Ctrl+C to exit..."
    Read-Host
}

# Function to check if a command exists
function Test-CommandExists {
    Param ($command)
    $exists = $false
    try {
        if (Get-Command $command -ErrorAction SilentlyContinue) {
            $exists = $true
        }
    } catch {}
    return $exists
}

# Install Chocolatey if not already installed
if (-not (Test-CommandExists choco)) {
    Write-Host "Installing Chocolatey package manager..." -ForegroundColor Yellow
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    try {
        Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
    } catch {
        Write-Error "Failed to install Chocolatey. Error: $_"
        exit 1
    }
} else {
    Write-Host "Chocolatey already installed" -ForegroundColor Green
}

# Packages to install
$packages = @(
    "visualstudio2022buildtools",   # VS Build Tools
    "visualstudio2022-workload-vctools",  # C++ workload
    "cmake",                        # Build system
    "git",                          # Version control
    "ninja",                        # Fast build system
    "vswhere",                      # Visual Studio locator
    "7zip"                          # For handling archives
)

# Install packages
foreach ($package in $packages) {
    if (-not (choco list --local-only --exact $package | Select-String "^$package\s+")) {
        Write-Host "Installing $package..." -ForegroundColor Yellow
        choco install $package -y
    } else {
        Write-Host "$package already installed" -ForegroundColor Green
    }
}

# Download and install dependencies (VST3 SDK)
$sdkDir = ".\external\SDKs"
if (-not (Test-Path $sdkDir)) {
    Write-Host "Creating SDKs directory..." -ForegroundColor Yellow
    mkdir -Force $sdkDir | Out-Null
}

# VST3 SDK
$vst3SdkPath = "$sdkDir\VST_SDK"
if (-not (Test-Path $vst3SdkPath)) {
    Write-Host "Downloading VST3 SDK..." -ForegroundColor Yellow
    $tempFile = "$env:TEMP\vst3sdk.zip"
    Invoke-WebRequest -Uri "https://download.steinberg.net/sdk_downloads/vst-sdk_3.7.5_build-25_2021-12-14.zip" -OutFile $tempFile
    Write-Host "Extracting VST3 SDK..." -ForegroundColor Yellow
    Expand-Archive -Path $tempFile -DestinationPath $sdkDir
    Rename-Item -Path "$sdkDir\VST_SDK" -NewName "VST3_SDK" -ErrorAction SilentlyContinue
    Remove-Item -Path $tempFile
    Write-Host "VST3 SDK installed" -ForegroundColor Green
} else {
    Write-Host "VST3 SDK already installed" -ForegroundColor Green
}

# Configure environment variables
Write-Host "Setting up environment variables..." -ForegroundColor Yellow
$env:VST3_SDK_PATH = (Resolve-Path "$sdkDir\VST3_SDK").Path
[Environment]::SetEnvironmentVariable("VST3_SDK_PATH", $env:VST3_SDK_PATH, "User")

# Generate Visual Studio solution
Write-Host "Generating Visual Studio solution..." -ForegroundColor Yellow
$buildDir = ".\build"
if (-not (Test-Path $buildDir)) {
    mkdir -Force $buildDir | Out-Null
}

Push-Location $buildDir
try {
    cmake -G "Visual Studio 17 2022" -A x64 ..
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to generate Visual Studio solution"
    } else {
        Write-Host "Visual Studio solution generated successfully" -ForegroundColor Green
    }
} finally {
    Pop-Location
}

Write-Host "Development environment setup complete!" -ForegroundColor Green
Write-Host "You can now open the solution in Visual Studio: $buildDir\VoicemeeterPluginHost.sln" -ForegroundColor Cyan
