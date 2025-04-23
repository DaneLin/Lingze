@echo off
setlocal EnableExtensions EnableDelayedExpansion
set CompilerExe="%VULKAN_SDK%\Bin\glslangValidator.exe"
set OptimizerExe="%VULKAN_SDK%\Bin\spirv-opt.exe"
set OptimizerConfig="OptimizerConfig.cfg"

@echo Creating spirv files...
if not exist "%~dp0spirv" mkdir "%~dp0spirv"
for /d /r "%~dp0glsl/" %%D in (*) do (
  set dirname=%%D
  set outdir=!dirname:%~dp0glsl=%~dp0spirv!
  if not exist "!outdir!" mkdir "!outdir!"
)

for /r "%~dp0glsl/" %%I in (*.vert) do (
  set outname=%%I
  set outname=!outname:%~dp0glsl=%~dp0spirv!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0glsl=%~dp0spirv!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    @echo Compiling new file %%I
    %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
  ) else (
    for %%J in ("%%I") do set glslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!glslTime!" gtr "!spvTime!" (
      @echo Updating file %%I
      %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
    )
  )
)

for /r "%~dp0glsl/" %%I in (*.frag) do (
  set outname=%%I
  set outname=!outname:%~dp0glsl=%~dp0spirv!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0glsl=%~dp0spirv!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    @echo Compiling new file %%I
    %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
  ) else (
    for %%J in ("%%I") do set glslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!glslTime!" gtr "!spvTime!" (
      @echo Updating file %%I
      %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
    )
  )
)

for /r "%~dp0glsl/" %%I in (*.comp) do (
  set outname=%%I
  set outname=!outname:%~dp0glsl=%~dp0spirv!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0glsl=%~dp0spirv!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    @echo Compiling new file %%I
    %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
  ) else (
    for %%J in ("%%I") do set glslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!glslTime!" gtr "!spvTime!" (
      @echo Updating file %%I
      %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
    )
  )
)
rem pause

rem 
rem %CompilerExe% -V glsl/fragmentShader.frag -o spirv/fragmentShader.spv
rem %CompilerExe% -V glsl/frameDescriptorLayout.comp -o spirv/frameDescriptorLayout.spv
rem %CompilerExe% -V glsl/passDescriptorLayout.comp -o spirv/passDescriptorLayout.spv

@echo.
@echo ========================================
@echo Shader compilation complete! All modified files have been updated.
@echo ========================================
@echo.
