# PowerShell script to clone the glTF-Sample-Assets repository
Write-Host "Cloning glTF-Sample-Assets repository..."

# Navigate to the script directory
$scriptPath = Split-Path -Parent $MyInvocation.MyCommand.Definition
Set-Location -Path $scriptPath

# Clone the repository
git clone https://github.com/KhronosGroup/glTF-Sample-Assets.git

Write-Host "Repository cloned successfully into $scriptPath\glTF-Sample-Assets" 