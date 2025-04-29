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
)

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

# Add SPIRV-Cross as a subdirectory to build it
add_subdirectory(spirv_cross)
add_subdirectory(meshoptimizer)

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
set(LINGZE_DEPS_LIB "lingze_deps;spirv-cross-core;spirv-cross-glsl;meshoptimizer" PARENT_SCOPE) 