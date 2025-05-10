# Dependency management

# Set library file paths
set(GLFW3_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/glfw/lib-vc2015/glfw3.lib" PARENT_SCOPE)

# Create an INTERFACE library to manage all include paths
add_library(deps_include INTERFACE)
target_include_directories(deps_include INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/tiny_obj_loader/include
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv_cross
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv_cross/include
    ${CMAKE_CURRENT_SOURCE_DIR}/meshoptimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/gli
    ${CMAKE_CURRENT_SOURCE_DIR}/glm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/glslang
    ${CMAKE_CURRENT_SOURCE_DIR}/fmt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf
)

# Configure fmt options
set(FMT_DOC OFF CACHE BOOL "Generate the fmt doc target" FORCE)
set(FMT_TEST OFF CACHE BOOL "Generate the fmt test target" FORCE)
set(FMT_INSTALL OFF CACHE BOOL "Generate the fmt install target" FORCE)

# Configure spdlog options
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "Build spdlog examples" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "Build spdlog tests" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "Build spdlog benchmarks" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "Generate the spdlog install target" FORCE)
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library" FORCE)

# Configure glslang options
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "Enable glslang binaries" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "Enables HLSL input support" FORCE)
set(ENABLE_CTEST OFF CACHE BOOL "Enables testing" FORCE)
set(BUILD_EXTERNAL OFF CACHE BOOL "Build external dependencies in /External" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "Enables spirv-tools optimizer" FORCE)

# Configure SPIRV-Cross using its own CMake build system
# Disable options we don't need to speed up build
set(SPIRV_CROSS_CLI OFF CACHE BOOL "Build the CLI binary" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "Enable SPIRV-Cross tests" FORCE)
set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "Enable MSL target support" FORCE)
set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "Enable HLSL target support" FORCE)
set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "Enable C++ target support" FORCE)
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "Enable JSON reflection target support" FORCE)
set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "Enable C API wrapper support" FORCE)
set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "Enable util module support" FORCE)
set(SPIRV_CROSS_STATIC ON CACHE BOOL "Build the C and C++ API as static libraries" FORCE)
set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "Skip installation targets" FORCE)

# Add subdirectories to build dependencies
add_subdirectory(fmt)
add_subdirectory(spdlog)
add_subdirectory(spirv_cross)
add_subdirectory(meshoptimizer)
add_subdirectory(glslang)

# Collect other dependency library source files (excluding SPIRV-Cross now)
file(GLOB_RECURSE JSONCPP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/*.cpp)
file(GLOB_RECURSE IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)

# Build other third-party libraries
add_library(lingze_deps STATIC 
    ${JSONCPP_SOURCES}
    ${IMGUI_SOURCES}
)

# Add correct include paths for third-party libraries
target_include_directories(lingze_deps PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# Export the libraries to parent scope
set(LINGZE_DEPS_LIB "lingze_deps;fmt;spdlog;spirv-cross-core;spirv-cross-glsl;meshoptimizer;glslang;SPIRV" PARENT_SCOPE) 