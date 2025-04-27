@echo off
setlocal EnableExtensions EnableDelayedExpansion
set CompilerExe="%VULKAN_SDK%\Bin\dxc.exe"
set OutputFormat="-spirv"
echo Creating SPIR-V files from HLSL...
if not exist "%~dp0spirv_hlsl" mkdir "%~dp0spirv_hlsl"
for /d /r "%~dp0hlsl/" %%D in (*) do (
  set dirname=%%D
  set outdir=!dirname:%~dp0hlsl=%~dp0spirv_hlsl!
  if not exist "!outdir!" mkdir "!outdir!"
)

rem Processing vertex shaders
for /r "%~dp0hlsl/" %%I in (*.vert.hlsl) do (
  set outname=%%I
  set outname=!outname:%~dp0hlsl=%~dp0spirv_hlsl!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0hlsl=%~dp0spirv_hlsl!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    echo Compiling new file %%I
    %CompilerExe% -E main -T vs_6_0 %OutputFormat% "%%I" -Fo !spvfile!
  ) else (
    for %%J in ("%%I") do set hlslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!hlslTime!" gtr "!spvTime!" (
      echo Updating file %%I
      %CompilerExe% -E main -T vs_6_0 %OutputFormat% "%%I" -Fo !spvfile!
    )
  )
)

rem Processing fragment shaders
for /r "%~dp0hlsl/" %%I in (*.frag.hlsl) do (
  set outname=%%I
  set outname=!outname:%~dp0hlsl=%~dp0spirv_hlsl!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0hlsl=%~dp0spirv_hlsl!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    echo Compiling new file %%I
    %CompilerExe% -E main -T ps_6_0 %OutputFormat% "%%I" -Fo !spvfile!
  ) else (
    for %%J in ("%%I") do set hlslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!hlslTime!" gtr "!spvTime!" (
      echo Updating file %%I
      %CompilerExe% -E main -T ps_6_0 %OutputFormat% "%%I" -Fo !spvfile!
    )
  )
)

rem Processing compute shaders
for /r "%~dp0hlsl/" %%I in (*.comp.hlsl) do (
  set outname=%%I
  set outname=!outname:%~dp0hlsl=%~dp0spirv_hlsl!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0hlsl=%~dp0spirv_hlsl!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    echo Compiling new file %%I
    %CompilerExe% -E main -T cs_6_0 %OutputFormat% "%%I" -Fo !spvfile!
  ) else (
    for %%J in ("%%I") do set hlslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!hlslTime!" gtr "!spvTime!" (
      echo Updating file %%I
      %CompilerExe% -E main -T cs_6_0 %OutputFormat% "%%I" -Fo !spvfile!
    )
  )
)

rem Processing mesh shaders
for /r "%~dp0hlsl/" %%I in (*.mesh.hlsl) do (
  set outname=%%I
  set outname=!outname:%~dp0hlsl=%~dp0spirv_hlsl!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0hlsl=%~dp0spirv_hlsl!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    echo Compiling new file %%I
    %CompilerExe% -E main -T ms_6_5 %OutputFormat% "%%I" -Fo !spvfile!
  ) else (
    for %%J in ("%%I") do set hlslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!hlslTime!" gtr "!spvTime!" (
      echo Updating file %%I
      %CompilerExe% -E main -T ms_6_5 %OutputFormat% "%%I" -Fo !spvfile!
    )
  )
)

rem Processing amplification shaders (equivalent to task shaders)
for /r "%~dp0hlsl/" %%I in (*.task.hlsl) do (
  set outname=%%I
  set outname=!outname:%~dp0hlsl=%~dp0spirv_hlsl!
  set outdir=%%~dpI
  set outdir=!outdir:%~dp0hlsl=%~dp0spirv_hlsl!
  if not exist "!outdir!" mkdir "!outdir!"
  set spvfile="!outname!.spv"
  if not exist !spvfile! (
    echo Compiling new file %%I
    %CompilerExe% -E main -T as_6_5 %OutputFormat% "%%I" -Fo !spvfile!
  ) else (
    for %%J in ("%%I") do set hlslTime=%%~tJ
    for %%K in (!spvfile!) do set spvTime=%%~tK
    if "!hlslTime!" gtr "!spvTime!" (
      echo Updating file %%I
      %CompilerExe% -E main -T as_6_5 %OutputFormat% "%%I" -Fo !spvfile!
    )
  )
)

echo HLSL shader compilation complete! All modified files have been updated in spirv_hlsl directory.
endlocal 