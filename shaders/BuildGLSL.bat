@echo off
setlocal EnableExtensions EnableDelayedExpansion
set CompilerExe="%VULKAN_SDK%\Bin\glslangValidator.exe"
set OptimizerExe="%VULKAN_SDK%\Bin\spirv-opt.exe"
set OptimizerConfig="OptimizerConfig.cfg"
echo Creating SPIR-V files from GLSL...
if not exist "%~dp0spirv_glsl" mkdir "%~dp0spirv_glsl"
for /d /r "%~dp0glsl/" %%D in (*) do (
  set dirname=%%D
  set outdir=!dirname:%~dp0glsl=%~dp0spirv_glsl!
  if not exist "!outdir!" mkdir "!outdir!"
)

rem Processing vertex shaders
for /r "%~dp0glsl/" %%I in (*.vert) do (
  set outname=%%I
  set outname=!outname:%~dp0glsl=%~dp0spirv_glsl!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0glsl=%~dp0spirv_glsl!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    echo Compiling new file %%I
    %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
  ) else (
    for %%J in ("%%I") do set glslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!glslTime!" gtr "!spvTime!" (
      echo Updating file %%I
      %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
    )
  )
)

rem Processing fragment shaders
for /r "%~dp0glsl/" %%I in (*.frag) do (
  set outname=%%I
  set outname=!outname:%~dp0glsl=%~dp0spirv_glsl!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0glsl=%~dp0spirv_glsl!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    echo Compiling new file %%I
    %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
  ) else (
    for %%J in ("%%I") do set glslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!glslTime!" gtr "!spvTime!" (
      echo Updating file %%I
      %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
    )
  )
)

rem Processing compute shaders
for /r "%~dp0glsl/" %%I in (*.comp) do (
  set outname=%%I
  set outname=!outname:%~dp0glsl=%~dp0spirv_glsl!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0glsl=%~dp0spirv_glsl!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    echo Compiling new file %%I
    %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
  ) else (
    for %%J in ("%%I") do set glslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!glslTime!" gtr "!spvTime!" (
      echo Updating file %%I
      %CompilerExe% -V "%%I" -l --target-env vulkan1.1 -o !spvfile!
    )
  )
)

rem Processing mesh shaders
for /r "%~dp0glsl/" %%I in (*.mesh) do (
  set outname=%%I
  set outname=!outname:%~dp0glsl=%~dp0spirv_glsl!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0glsl=%~dp0spirv_glsl!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    echo Compiling new file %%I
    %CompilerExe% -V "%%I" -l --target-env vulkan1.2 -o !spvfile!
  ) else (
    for %%J in ("%%I") do set glslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!glslTime!" gtr "!spvTime!" (
      echo Updating file %%I
      %CompilerExe% -V "%%I" -l --target-env vulkan1.2 -o !spvfile!
    )
  )
)

rem Processing task shaders
for /r "%~dp0glsl/" %%I in (*.task) do (
  set outname=%%I
  set outname=!outname:%~dp0glsl=%~dp0spirv_glsl!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0glsl=%~dp0spirv_glsl!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    echo Compiling new file %%I
    %CompilerExe% -V "%%I" -l --target-env vulkan1.2 -o !spvfile!
  ) else (
    for %%J in ("%%I") do set glslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!glslTime!" gtr "!spvTime!" (
      echo Updating file %%I
      %CompilerExe% -V "%%I" -l --target-env vulkan1.2 -o !spvfile!
    )
  )
)

echo GLSL shader compilation complete! All modified files have been updated in spirv_glsl directory.
endlocal 