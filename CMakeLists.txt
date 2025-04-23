# CMakeList.txt: Lingze 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(LingzeEngineCMake VERSION 0.1.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

find_package(Vulkan)
message(STATUS "Found Vulkan in ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Found Vulkan libs in ${Vulkan_LIBRARIES}")

# 添加deps子目录，依赖库的配置将在deps/CMakeLists.txt中管理
add_subdirectory(deps)

#find_package(GLM)
#find_package(GLFW3)

# 添加项目包含路径
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${Vulkan_INCLUDE_DIRS}")

#include_directories ("${CMAKE_SOURCE_DIR}/deps/glm/include")
#include_directories ("${CMAKE_SOURCE_DIR}/deps/glfw/include")

# 定义着色器路径，以便于在代码中使用
add_compile_definitions(
    SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders"
    SHADER_SPIRV_DIR="${CMAKE_SOURCE_DIR}/shaders/spirv"
    SHADER_GLSL_DIR="${CMAKE_SOURCE_DIR}/shaders/glsl"
)

# Define shader compilation custom target
add_custom_target(
    CompileShaders
    COMMAND ${CMAKE_COMMAND} -E echo "Compiling shaders..."
    COMMAND cmd.exe /c "${CMAKE_SOURCE_DIR}/shaders/buildShaders.bat"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Compiling modified shader files to SPIR-V"
    VERBATIM
)

# 显式列出所有源文件和头文件
# 主目录文件
set(lingze_main_sources
    src/Lingze.cpp
)

set(lingze_main_headers
    # 主目录下没有.h文件
)

# 后端文件
set(lingze_backend_sources
    src/backend/Buffer.cpp
    src/backend/Core.cpp
    src/backend/CpuProfiler.cpp
    src/backend/DescriptorSetCache.cpp
    src/backend/Framebuffer.cpp
    src/backend/GpuProfiler.cpp
    src/backend/Handles.cpp
    src/backend/Image.cpp
    src/backend/ImageLoader.cpp
    src/backend/ImageView.cpp
    src/backend/ImGuiProfilerRenderer.cpp
    src/backend/LingzeVK.cpp
    src/backend/Pipeline.cpp
    src/backend/PipelineCache.cpp
    src/backend/Pool.cpp
    src/backend/PresentQueue.cpp
    src/backend/ProfilerTask.cpp
    src/backend/QueueIndices.cpp
    src/backend/RenderGraph.cpp
    src/backend/RenderPass.cpp
    src/backend/RenderPassCache.cpp
    src/backend/Sampler.cpp
    src/backend/ShaderMemoryPool.cpp
    src/backend/ShaderModule.cpp
    src/backend/ShaderProgram.cpp
    src/backend/StagedResources.cpp
    src/backend/Surface.cpp
    src/backend/Swapchain.cpp
    src/backend/Synchronization.cpp
    src/backend/TimestampQuery.cpp
    src/backend/VertexDeclaration.cpp
)

set(lingze_backend_headers
    src/backend/Buffer.h
    src/backend/Core.h
    src/backend/CpuProfiler.h
    src/backend/DescriptorSetCache.h
    src/backend/Framebuffer.h
    src/backend/GpuProfiler.h
    src/backend/Handles.h
    src/backend/Image.h
    src/backend/ImageLoader.h
    src/backend/ImageView.h
    src/backend/ImGuiProfilerRenderer.h
    src/backend/LingzeVK.h
    src/backend/Pipeline.h
    src/backend/PipelineCache.h
    src/backend/Pool.h
    src/backend/PresentQueue.h
    src/backend/ProfilerTask.h
    src/backend/QueueIndices.h
    src/backend/RenderGraph.h
    src/backend/RenderPass.h
    src/backend/RenderPassCache.h
    src/backend/Sampler.h
    src/backend/ShaderMemoryPool.h
    src/backend/ShaderModule.h
    src/backend/ShaderProgram.h
    src/backend/StagedResources.h
    src/backend/Surface.h
    src/backend/Swapchain.h
    src/backend/Synchronization.h
    src/backend/TimestampQuery.h
    src/backend/VertexDeclaration.h
)

# 渲染通用文件
set(lingze_render_common_sources
    src/render/common/ImguiRenderer.cpp
)

set(lingze_render_common_headers
    src/render/common/ImguiRenderer.h
)

# 渲染器文件
set(lingze_renderers_sources
    # render/renderers 目录下没有 .cpp 文件
)

set(lingze_renderers_headers
    src/render/renderers/BaseRenderer.h
)

# Shader文件
set(shader_files
    shaders/glsl/ImGui/ImGui.frag
    shaders/glsl/ImGui/ImGui.vert
    shaders/buildShaders.bat
)

# 所有源文件和头文件列表
set(lingze_sources 
    ${lingze_main_sources}
    ${lingze_backend_sources}
    ${lingze_render_common_sources}
    ${lingze_renderers_sources}
)

set(lingze_headers
    ${lingze_main_headers}
    ${lingze_backend_headers}
    ${lingze_render_common_headers}
    ${lingze_renderers_headers}
)

add_executable(LingzeEngine)

# Add dependency on shader compilation
add_dependencies(LingzeEngine CompileShaders)

# 为源文件创建源码组，以在IDE中更好地组织文件
source_group("Main" FILES ${lingze_main_sources} ${lingze_main_headers})
source_group("Backend" FILES ${lingze_backend_sources} ${lingze_backend_headers})
source_group("Render\\Common" FILES ${lingze_render_common_sources} ${lingze_render_common_headers})
source_group("Render\\Renderers" FILES ${lingze_renderers_sources} ${lingze_renderers_headers})
source_group("Shaders" FILES ${shader_files})

target_sources(LingzeEngine PRIVATE ${DEP_SOURCES} ${lingze_sources} ${lingze_headers} ${shader_files})
target_link_libraries(LingzeEngine ${Vulkan_LIBRARIES} ${GLFW3_LIBRARIES} deps_include)
set_target_properties(LingzeEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/cmaked")
set_target_properties(LingzeEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/cmake")

# TODO: 如有需要，请添加测试并安装目标。
