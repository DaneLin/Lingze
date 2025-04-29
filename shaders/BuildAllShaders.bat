@echo off
echo Starting shader compilation...

echo Compiling GLSL shaders...
call "%~dp0BuildGLSL.bat"

echo Compiling HLSL shaders...
call "%~dp0BuildHLSL.bat"

echo All shader compilation complete! GLSL compiled to spirv_glsl directory, HLSL compiled to spirv_hlsl directory. 