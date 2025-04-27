# HLSL着色器使用指南

本项目现在支持使用HLSL（高级着色语言）编写着色器，并通过Microsoft的DXC编译器将其编译为SPIR-V格式，以便在Vulkan中使用。

## 前提条件

1. 确保已安装Vulkan SDK
2. DXC编译器通常包含在Vulkan SDK中（位于`%VULKAN_SDK%\Bin\dxc.exe`）

## 文件命名规范

HLSL着色器文件应使用以下命名规范：

- 顶点着色器: `*.vert.hlsl`
- 片段着色器: `*.frag.hlsl`
- 计算着色器: `*.comp.hlsl`
- 网格着色器: `*.mesh.hlsl`
- 任务/放大着色器: `*.task.hlsl`

## 目录结构

项目现在使用以下目录结构来区分不同源语言的着色器：

- `shaders/glsl/` - GLSL源码目录
- `shaders/hlsl/` - HLSL源码目录
- `shaders/spirv_glsl/` - GLSL编译的SPIR-V文件
- `shaders/spirv_hlsl/` - HLSL编译的SPIR-V文件

## 编译方法

有两种方式编译着色器：

1. 仅编译HLSL着色器:
   ```
   cd shaders
   BuildHLSL.bat
   ```

2. 编译所有着色器（GLSL和HLSL）:
   ```
   cd shaders
   BuildAllShaders.bat
   ```

## GLSL到HLSL的转换指南

以下是从GLSL转换到HLSL的一些基本规则：

### 着色器入口点

- GLSL: `void main()`
- HLSL: `ReturnType main(inputs) : SEMANTIC`

### 顶点属性

- GLSL: `layout(location = 0) in vec3 inPosition;`
- HLSL: 通过`SV_VertexID`或结构体传递

### 输出变量

- GLSL: `layout(location = 0) out vec4 outColor;`
- HLSL: 使用`SV_Target`语义和返回值，或通过结构体作为`return`

### 数据类型

- GLSL: `vec2`, `vec3`, `vec4`
- HLSL: `float2`, `float3`, `float4`

### 纹理采样

- GLSL: `texture(sampler2D, uv)`
- HLSL: `Texture2D.Sample(Sampler, uv)`

### 内置变量

- GLSL: `gl_Position`, `gl_VertexID`, `gl_FragCoord`
- HLSL: `output.position`（带有`SV_Position`）, `SV_VertexID`, `SV_Position`

## 示例

查看`shaders/hlsl/Simple`目录中的示例着色器，了解如何将GLSL着色器转换为HLSL。 

# HLSL Shader Usage Guide

This project now supports writing shaders in HLSL (High-Level Shading Language) and compiling them to SPIR-V format for use with Vulkan using Microsoft's DXC compiler.

## Prerequisites

1. Ensure you have the Vulkan SDK installed
2. The DXC compiler is typically included in the Vulkan SDK (located at `%VULKAN_SDK%\Bin\dxc.exe`)

## File Naming Conventions

HLSL shader files should use the following naming conventions:

- Vertex shaders: `*.vert.hlsl`
- Fragment shaders: `*.frag.hlsl`
- Compute shaders: `*.comp.hlsl`
- Mesh shaders: `*.mesh.hlsl`
- Task/Amplification shaders: `*.task.hlsl`

## Directory Structure

The project now uses the following directory structure to distinguish between different source languages:

- `shaders/glsl/` - GLSL source directory
- `shaders/hlsl/` - HLSL source directory
- `shaders/spirv_glsl/` - GLSL compiled SPIR-V files
- `shaders/spirv_hlsl/` - HLSL compiled SPIR-V files

## Compilation Methods

There are two ways to compile shaders:

1. Compile only HLSL shaders:
   ```
   cd shaders
   BuildHLSL.bat
   ```

2. Compile all shaders (GLSL and HLSL):
   ```
   cd shaders
   BuildAllShaders.bat
   ```

## GLSL to HLSL Conversion Guide

Here are some basic rules for converting from GLSL to HLSL:

### Shader Entry Points

- GLSL: `void main()`
- HLSL: `ReturnType main(inputs) : SEMANTIC`

### Vertex Attributes

- GLSL: `layout(location = 0) in vec3 inPosition;`
- HLSL: Pass through `SV_VertexID` or structures

### Output Variables

- GLSL: `layout(location = 0) out vec4 outColor;`
- HLSL: Use the `SV_Target` semantic and return value, or through structures as `return`

### Data Types

- GLSL: `vec2`, `vec3`, `vec4`
- HLSL: `float2`, `float3`, `float4`

### Texture Sampling

- GLSL: `texture(sampler2D, uv)`
- HLSL: `Texture2D.Sample(Sampler, uv)`

### Built-in Variables

- GLSL: `gl_Position`, `gl_VertexID`, `gl_FragCoord`
- HLSL: `output.position` (with `SV_Position`), `SV_VertexID`, `SV_Position`

## Examples

Check the shader examples in the `shaders/hlsl/Simple` directory to learn how to convert GLSL shaders to HLSL. 