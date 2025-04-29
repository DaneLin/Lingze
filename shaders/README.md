# 着色器构建系统

本项目支持GLSL和HLSL两种着色器语言，并将它们编译为SPIR-V格式以在Vulkan中使用。

## 目录结构

项目使用以下目录结构来管理不同的着色器语言：

- `shaders/glsl/` - GLSL源码目录
- `shaders/hlsl/` - HLSL源码目录
- `shaders/spirv_glsl/` - GLSL编译的SPIR-V文件
- `shaders/spirv_hlsl/` - HLSL编译的SPIR-V文件

## 文件命名规范

### GLSL文件

- 顶点着色器: `*.vert`
- 片段着色器: `*.frag`
- 计算着色器: `*.comp`
- 网格着色器: `*.mesh`
- 任务着色器: `*.task`

### HLSL文件

- 顶点着色器: `*.vert.hlsl`
- 片段着色器: `*.frag.hlsl`
- 计算着色器: `*.comp.hlsl`
- 网格着色器: `*.mesh.hlsl`
- 任务/放大着色器: `*.task.hlsl`

## 构建脚本

项目提供了多个脚本用于编译着色器：

1. `BuildGLSL.bat` - 将GLSL着色器编译到spirv_glsl目录
2. `BuildHLSL.bat` - 将HLSL着色器编译到spirv_hlsl目录
3. `BuildAllShaders.bat` - 编译所有着色器（同时处理GLSL和HLSL）

## 使用方法

### 编译所有着色器

```
cd shaders
BuildAllShaders.bat
```

### 仅编译GLSL着色器

```
cd shaders
BuildGLSL.bat
```

### 仅编译HLSL着色器

```
cd shaders
BuildHLSL.bat
```

## 更多信息

- 关于HLSL着色器和GLSL到HLSL的转换指南，请查看[HLSL_README_CN.md](HLSL_README_CN.md)。
- 要了解如何在代码中使用这些编译后的着色器，请参考项目的渲染系统文档。 

# Shader Build System

This project supports both GLSL and HLSL shader languages, compiling them to SPIR-V format for use with Vulkan.

## Directory Structure

The project uses the following directory structure to manage different shader languages:

- `shaders/glsl/` - GLSL source directory
- `shaders/hlsl/` - HLSL source directory
- `shaders/spirv_glsl/` - GLSL compiled SPIR-V files
- `shaders/spirv_hlsl/` - HLSL compiled SPIR-V files

## File Naming Conventions

### GLSL Files

- Vertex shaders: `*.vert`
- Fragment shaders: `*.frag`
- Compute shaders: `*.comp`
- Mesh shaders: `*.mesh`
- Task shaders: `*.task`

### HLSL Files

- Vertex shaders: `*.vert.hlsl`
- Fragment shaders: `*.frag.hlsl`
- Compute shaders: `*.comp.hlsl`
- Mesh shaders: `*.mesh.hlsl`
- Task/Amplification shaders: `*.task.hlsl`

## Build Scripts

The project provides multiple scripts for compiling shaders:

1. `BuildGLSL.bat` - Compiles GLSL shaders to the spirv_glsl directory
2. `BuildHLSL.bat` - Compiles HLSL shaders to the spirv_hlsl directory
3. `BuildAllShaders.bat` - Compiles all shaders (processes both GLSL and HLSL)

## Usage

### Compile All Shaders

```
cd shaders
BuildAllShaders.bat
```

### Compile Only GLSL Shaders

```
cd shaders
BuildGLSL.bat
```

### Compile Only HLSL Shaders

```
cd shaders
BuildHLSL.bat
```

## More Information

- For information about HLSL shaders and GLSL to HLSL conversion guidelines, see [HLSL_README.md](HLSL_README.md).
- To learn how to use these compiled shaders in code, refer to the project's rendering system documentation. 