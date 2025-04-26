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

rem 处理顶点着色器
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

rem 处理片段着色器
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

rem 处理计算着色器
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

rem 处理mesh着色器
for /r "%~dp0glsl/" %%I in (*.mesh) do (
  set outname=%%I
  set outname=!outname:%~dp0glsl=%~dp0spirv!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0glsl=%~dp0spirv!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    @echo Compiling new file %%I
    %CompilerExe% -V "%%I" -l --target-env vulkan1.2 -o !spvfile!
  ) else (
    for %%J in ("%%I") do set glslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!glslTime!" gtr "!spvTime!" (
      @echo Updating file %%I
      %CompilerExe% -V "%%I" -l --target-env vulkan1.2 -o !spvfile!
    )
  )
)

rem 处理task着色器
for /r "%~dp0glsl/" %%I in (*.task) do (
  set outname=%%I
  set outname=!outname:%~dp0glsl=%~dp0spirv!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0glsl=%~dp0spirv!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    @echo Compiling new file %%I
    %CompilerExe% -V "%%I" -l --target-env vulkan1.2 -o !spvfile!
  ) else (
    for %%J in ("%%I") do set glslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!glslTime!" gtr "!spvTime!" (
      @echo Updating file %%I
      %CompilerExe% -V "%%I" -l --target-env vulkan1.2 -o !spvfile!
    )
  )
)

@echo Shader compilation complete! All modified files have been updated.