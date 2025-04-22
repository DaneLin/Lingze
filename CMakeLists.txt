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

set(TINYOBJ_INCLUDE_DIR ./deps/tiny_obj_loader/include)
set(SPIRVCROSS_INCLUDE_DIR ./deps/spirv-cross)
set(GLI_INCLUDE_DIR ./deps/gli)
set(GLM_INCLUDE_DIR ./deps/glm/include)
set(GLFW3_INCLUDE_DIR ./deps/glfw/include)
set(JSONCPP_INCLUDE_DIR ./deps/jsoncpp)
set(IMGUI_INCLUDE_DIR ./deps/imgui)

set(GLFW3_LIBRARIES "${CMAKE_SOURCE_DIR}/deps/glfw/lib-vc2015/glfw3.lib")
#find_package(GLM)
#find_package(GLFW3)

include_directories ("${Vulkan_INCLUDE_DIRS}")
include_directories ("${GLM_INCLUDE_DIR}")
include_directories ("${GLFW3_INCLUDE_DIR}")
include_directories ("${SPIRVCROSS_INCLUDE_DIR}")
include_directories ("${GLI_INCLUDE_DIR}")
include_directories ("${TINYOBJ_INCLUDE_DIR}")
include_directories ("${JSONCPP_INCLUDE_DIR}")
include_directories ("${IMGUI_INCLUDE_DIR}")

#include_directories ("${CMAKE_SOURCE_DIR}/deps/glm/include")
#include_directories ("${CMAKE_SOURCE_DIR}/deps/glfw/include")

file(GLOB_RECURSE dep_sources ${SPIRVCROSS_INCLUDE_DIR}/*.cpp ${TINYOBJ_INCLUDE_DIR}/*.cc ${JSONCPP_INCLUDE_DIR}/*.cpp ${IMGUI_INCLUDE_DIR}/*.cpp)
file(GLOB_RECURSE lingze_sources ./src/*.cpp)
file(GLOB_RECURSE lingze_headers ./src/*.h)

add_executable(LingzeEngine)

#source_group(TREE src/)
target_sources(LingzeEngine PRIVATE ${dep_sources} ${lingze_sources} ${lingze_headers})
target_link_libraries(LingzeEngine ${Vulkan_LIBRARIES} ${GLFW3_LIBRARIES})
set_target_properties(LingzeEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/cmaked")
set_target_properties(LingzeEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/cmake")

# TODO: 如有需要，请添加测试并安装目标。
