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

# 只保留Vulkan的包含路径，其他依赖库通过deps_include管理
include_directories ("${Vulkan_INCLUDE_DIRS}")

#include_directories ("${CMAKE_SOURCE_DIR}/deps/glm/include")
#include_directories ("${CMAKE_SOURCE_DIR}/deps/glfw/include")

# Define shader compilation custom target
add_custom_target(
    CompileShaders
    COMMAND ${CMAKE_COMMAND} -E echo "Compiling shaders..."
    COMMAND cmd.exe /c "${CMAKE_SOURCE_DIR}/shaders/buildShaders.bat"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Compiling modified shader files to SPIR-V"
    VERBATIM
)

file(GLOB_RECURSE lingze_sources ./src/*.cpp)
file(GLOB_RECURSE lingze_headers ./src/*.h)

add_executable(LingzeEngine)

# Add dependency on shader compilation
add_dependencies(LingzeEngine CompileShaders)

#source_group(TREE src/)
target_sources(LingzeEngine PRIVATE ${DEP_SOURCES} ${lingze_sources} ${lingze_headers})
target_link_libraries(LingzeEngine ${Vulkan_LIBRARIES} ${GLFW3_LIBRARIES} deps_include)
set_target_properties(LingzeEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/cmaked")
set_target_properties(LingzeEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/cmake")

# TODO: 如有需要，请添加测试并安装目标。
