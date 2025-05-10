#!/bin/bash

# Script to clone the glTF-Sample-Assets repository
echo "Cloning glTF-Sample-Assets repository..."

# Navigate to the data directory
cd "$(dirname "$0")"

# Clone the repository
git clone https://github.com/KhronosGroup/glTF-Sample-Assets.git

echo "Repository cloned successfully into $(pwd)/glTF-Sample-Assets" 