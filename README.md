# Lingze (灵泽) Vulkan 引擎

<div align="right">
  <a href="#readme-english">English</a> | <b>中文</b>
</div>

## 简介

Lingze（灵泽）是一个基于Vulkan API的轻量级渲染引擎，专注于提供高效、灵活的图形渲染功能。该引擎封装了Vulkan的复杂底层细节，提供简洁的API接口，简化了渲染管线的构建过程，同时保留了对底层API的直接访问能力，用于个人图形学习和项目开发。

## 核心功能

- **Vulkan抽象层**：完整封装Vulkan实例、物理/逻辑设备、命令缓冲区和同步原语
- **渲染同步机制**：完整的图像和缓冲区同步系统，支持不同队列族之间的操作协调
- **渲染图系统**：基于现代渲染图架构，支持高效的渲染通道管理和资源依赖跟踪
- **资源管理**：智能的缓冲区和图像资源管理，包括描述符集缓存和管线缓存
- **场景系统**：支持从JSON文件加载和管理3D场景，包括网格、材质和变换
- **ImGui集成**：内置Dear ImGui支持，便于创建调试界面和工具

## 渲染功能

- **可编程着色器管线**：完整支持顶点、片段着色器，便于实现自定义渲染效果
- **内存优化**：高效的内存管理，支持统一内存访问和缓冲区复用
- **渲染通道抽象**：简化的渲染通道和子通道创建，支持多种附件类型
- **帧缓冲管理**：统一的帧缓冲管理接口，与渲染通道无缝集成
- **类型安全API**：使用C++枚举类和强类型设计提供类型安全的API

## 项目结构

项目采用模块化设计，主要包含以下组件：

```
Lingze/
├── src/                      # 源代码目录
│   ├── backend/              # 核心引擎后端
│   │   ├── Core.h/cpp        # 引擎核心类，管理Vulkan实例和设备
│   │   ├── RenderGraph.h/cpp # 渲染图系统
│   │   ├── App.h/cpp         # 应用程序基类
│   │   ├── Buffer.h/cpp      # 缓冲区管理
│   │   ├── Image.h/cpp       # 图像资源管理
│   │   └── ...               # 其他核心组件
│   ├── render/               # 渲染系统
│   │   ├── common/           # 通用渲染组件
│   │   └── renderers/        # 不同类型的渲染器实现
│   │       ├── BaseRenderer.h        # 渲染器基类
│   │       ├── SimpleRenderer.h/cpp  # 简单三角形渲染器
│   │       └── BasicShapeRenderer.h/cpp # 基础几何体渲染器
│   ├── scene/                # 场景管理系统
│   │   ├── Scene.h/cpp       # 场景类，管理对象和材质
│   │   └── Mesh.h/cpp        # 网格数据和加载
│   └── application/          # 示例应用程序
│       ├── EntryPoint.h      # 应用程序入口点宏
│       ├── SimpleTriangleApp.h/cpp  # 三角形渲染示例
│       └── BasicShapeApp.h/cpp      # 基础几何体示例
├── shaders/                  # 着色器目录
│   ├── glsl/                 # GLSL源代码
│   └── spirv/                # 编译后的SPIR-V字节码
├── deps/                     # 第三方依赖
├── data/                     # 资源数据（模型、纹理等）
└── CMakeLists.txt            # CMake构建配置
```

项目使用以下示例应用程序展示引擎功能：

- **SimpleTriangleApp**：演示最基本的三角形渲染，展示Vulkan渲染管线的基础用法
- **BasicShapeApp**：展示如何加载和渲染3D模型，包括立方体等基础几何体

通过继承`lz::App`基类并实现必要的方法，可以创建自定义渲染应用。

## 安装

### 前提条件

- C++17或更高版本的编译器
- Vulkan SDK 1.2或更高版本
- CMake 3.12或更高版本
- GLFW3库 (窗口管理)

### 构建过程

```bash
git clone https://github.com/yourusername/lingze.git
cd lingze
mkdir build && cd build
cmake ..
cmake --build .
```

## 功能实现状态


- **渲染特性**
  - [ ] 渲染图架构
  - [ ] PBR材质系统
  - [ ] 实时阴影
  - [ ] SSAO
  - [ ] 延迟渲染
  - [ ] Mesh Shader
  - [ ] 计算着色器
  - [ ] 光线追踪

- **资源系统**
  - [ ] 自动内存管理
  - [ ] 资源热重载

- **工具与性能**
  - [x] 基础ImGui集成
  - [ ] 高级性能分析
  - [ ] 多线程命令生成


- **场景管理**
  - [x] 网格加载
    - [x] OBJ模型导入
  - [ ] 高级场景图
  - [ ] 加速结构
  - [ ] 实例化渲染
  - [ ] 场景编辑器

## 贡献

欢迎贡献！请查看[贡献指南](CONTRIBUTING.md)了解如何参与项目开发。

## 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

<a name="readme-english"></a>

# Lingze (灵泽) Vulkan Engine

<div align="right">
  <b>English</b> | <a href="#">中文</a>
</div>

## Introduction

Lingze (灵泽) is a lightweight rendering engine based on the Vulkan API, focused on providing efficient and flexible graphics rendering capabilities. The engine encapsulates the complex low-level details of Vulkan, provides a clean API interface, simplifies the rendering pipeline construction process, and retains direct access to the underlying API, making it suitable for graphics learning and project development.

## Core Features

- **Vulkan Abstraction**: Complete encapsulation of Vulkan instances, physical/logical devices, command buffers, and synchronization primitives
- **Rendering Synchronization**: Complete image and buffer synchronization system, supporting operation coordination between different queue families
- **Render Graph System**: Based on modern render graph architecture, supporting efficient render pass management and resource dependency tracking
- **Resource Management**: Smart buffer and image resource management, including descriptor set caching and pipeline caching
- **Scene System**: Support for loading and managing 3D scenes from JSON files, including meshes, materials, and transformations
- **ImGui Integration**: Built-in Dear ImGui support for easy creation of debug interfaces and tools

## Rendering Features

- **Programmable Shader Pipeline**: Complete support for vertex and fragment shaders, facilitating custom rendering effects
- **Memory Optimization**: Efficient memory management with support for unified memory access and buffer reuse
- **Render Pass Abstraction**: Simplified render pass and subpass creation, supporting various attachment types
- **Framebuffer Management**: Unified framebuffer management interface, seamlessly integrated with render passes
- **Type-Safe API**: Type-safe API using C++ enum classes and strong typing design

## Project Structure

The project uses a modular design with a clear code structure, comprised of the following main components:

```
Lingze/
├── src/                      # Source code directory
│   ├── backend/              # Core engine backend
│   │   ├── Core.h/cpp        # Engine core class, manages Vulkan instances and devices
│   │   ├── RenderGraph.h/cpp # Render graph system
│   │   ├── App.h/cpp         # Application base class
│   │   ├── Buffer.h/cpp      # Buffer management
│   │   ├── Image.h/cpp       # Image resource management
│   │   └── ...               # Other core components
│   ├── render/               # Rendering system
│   │   ├── common/           # Common rendering components
│   │   └── renderers/        # Different renderer implementations
│   │       ├── BaseRenderer.h        # Renderer base class
│   │       ├── SimpleRenderer.h/cpp  # Simple triangle renderer
│   │       └── BasicShapeRenderer.h/cpp # Basic geometry renderer
│   ├── scene/                # Scene management system
│   │   ├── Scene.h/cpp       # Scene class, manages objects and materials
│   │   └── Mesh.h/cpp        # Mesh data and loading
│   └── application/          # Example applications
│       ├── EntryPoint.h      # Application entry point macro
│       ├── SimpleTriangleApp.h/cpp  # Triangle rendering example
│       └── BasicShapeApp.h/cpp      # Basic geometry example
├── shaders/                  # Shader directory
│   ├── glsl/                 # GLSL source code
│   └── spirv/                # Compiled SPIR-V bytecode
├── deps/                     # Third-party dependencies
├── data/                     # Resource data (models, textures, etc.)
└── CMakeLists.txt            # CMake build configuration
```

The engine functionality is demonstrated through the following example applications:

- **SimpleTriangleApp**: Demonstrates the most basic triangle rendering, showcasing the fundamentals of the Vulkan rendering pipeline
- **BasicShapeApp**: Shows how to load and render 3D models, including basic geometric shapes like cubes

Custom rendering applications can be created by inheriting from the `lz::App` base class and implementing the necessary methods.

## Installation

### Prerequisites

- C++17 or higher compiler
- Vulkan SDK 1.2 or higher
- CMake 3.12 or higher
- GLFW3 library (window management)

### Build Process

```bash
git clone https://github.com/yourusername/lingze.git
cd lingze
mkdir build && cd build
cmake ..
cmake --build .
```

## Feature Implementation Status


- **Rendering Features**
  - [ ] Render graph architecture
  - [ ] PBR material system
  - [ ] Real-time shadows
  - [ ] SSAO
  - [ ] Deferred rendering
  - [ ] Mesh Shader
  - [ ] Compute shaders
  - [ ] Ray tracing

- **Resource System**
  - [ ] Automatic memory management
  - [ ] Resource hot reloading

- **Tools & Performance**
  - [x] Basic ImGui integration
  - [ ] Advanced profiling
  - [ ] Multi-threaded command generation


- **Scene Management**
  - [x] Mesh loading
    - [x] OBJ model import
  - [ ] Advanced scene graph
  - [ ] Acceleration structures
  - [ ] Instanced rendering
  - [ ] Scene editor

## Contributing

Contributions are welcome! Please check out the [contribution guidelines](CONTRIBUTING.md) for details on how to participate in the project development.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details