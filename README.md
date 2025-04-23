# Lingze Vulkan 引擎

<div align="right">
  <a href="#readme-english">English</a> | <b>中文</b>
</div>

## 简介

Lingze是一个基于Vulkan API的轻量级渲染引擎，专注于提供高效、灵活的图形渲染功能。该引擎封装了Vulkan的底层细节，简化了渲染管线的构建过程，同时保留了对底层API的直接访问能力。

## 特性

- **渲染同步机制**：完整的图像和缓冲区同步系统，支持不同队列族之间的操作协调
- **渲染通道管理**：简化的渲染通道和子通道创建，支持多种附件类型
- **帧缓冲抽象**：统一的帧缓冲管理接口，与渲染通道无缝集成
- **类型安全的枚举**：使用C++枚举类提供类型安全的API

## 安装

### 前提条件

- C++17或更高版本的编译器
- Vulkan SDK 1.2或更高版本
- CMake 3.12或更高版本

### 构建过程

```bash
git clone https://github.com/yourusername/lingze.git
cd lingze
mkdir build && cd build
cmake ..
cmake --build .
```

## 未来计划

- [ ] 更智能的同步障碍管理
- [ ] 计算管线支持
- [ ] 多线程命令缓冲生成
- [ ] 自动内存管理

## 贡献

欢迎贡献！请查看[贡献指南](CONTRIBUTING.md)了解如何参与项目开发。

## 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

<a name="readme-english"></a>

# Lingze Vulkan Engine

<div align="right">
  <b>English</b> | <a href="#">中文</a>
</div>

## Introduction

Lingze is a lightweight rendering engine based on the Vulkan API, focused on providing efficient and flexible graphics rendering capabilities. The engine encapsulates the low-level details of Vulkan, simplifying the construction process of the rendering pipeline while retaining direct access to the underlying API.

## Features

- **Rendering Synchronization**: Complete image and buffer synchronization system, supporting operation coordination between different queue families
- **Render Pass Management**: Simplified render pass and subpass creation, supporting various attachment types
- **Framebuffer Abstraction**: Unified framebuffer management interface, seamlessly integrated with render passes
- **Type-Safe Enums**: Type-safe API using C++ enum classes

## Installation

### Prerequisites

- C++17 or higher compiler
- Vulkan SDK 1.2 or higher
- CMake 3.12 or higher

### Build Process

```bash
git clone https://github.com/yourusername/lingze.git
cd lingze
mkdir build && cd build
cmake ..
cmake --build .
```

## Future Plans

- [ ] Smarter synchronization barrier management
- [ ] Compute pipeline support
- [ ] Multi-threaded command buffer generation
- [ ] Automatic memory management

## Contributing

Contributions are welcome! Please check out the [contribution guidelines](CONTRIBUTING.md) for details on how to participate in the project development.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details