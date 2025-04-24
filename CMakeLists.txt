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

# 设置C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# 启用VS的文件夹和过滤器
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 配置VS的CMake设置，使文件添加后自动更新
if(MSVC)
  set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD 1)
  set(CMAKE_SUPPRESS_REGENERATION OFF)
endif()

find_package(Vulkan)
message(STATUS "Found Vulkan in ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Found Vulkan libs in ${Vulkan_LIBRARIES}")

# 添加deps子目录，依赖库的配置将在deps/CMakeLists.txt中管理
add_subdirectory(deps)

# 添加项目包含路径
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${Vulkan_INCLUDE_DIRS}")

# 定义着色器路径，以便于在代码中使用
add_compile_definitions(
    SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/"
    SHADER_SPIRV_DIR="${CMAKE_SOURCE_DIR}/shaders/spirv/"
    SHADER_GLSL_DIR="${CMAKE_SOURCE_DIR}/shaders/glsl/"
    SCENE_DIR="${CMAKE_SOURCE_DIR}/data/scenes/"
    # 解决GLFW和Windows头文件宏冲突
    GLFW_INCLUDE_NONE
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

# 使用GLOB_RECURSE自动查找所有源文件和头文件
# 主目录文件（只包含根目录下的.cpp文件）
file(GLOB lingze_main_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB lingze_main_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.h")

# 应用程序文件
file(GLOB_RECURSE lingze_application_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/*.cpp")
file(GLOB_RECURSE lingze_application_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/*.h")

# 后端文件
file(GLOB_RECURSE lingze_backend_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/backend/*.cpp")
file(GLOB_RECURSE lingze_backend_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/backend/*.h")

# 渲染通用文件
file(GLOB_RECURSE lingze_render_common_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/render/common/*.cpp")
file(GLOB_RECURSE lingze_render_common_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/render/common/*.h")

# 渲染器文件
file(GLOB_RECURSE lingze_renderers_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/render/renderers/*.cpp")
file(GLOB_RECURSE lingze_renderers_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/render/renderers/*.h")

# 场景文件
file(GLOB_RECURSE lingze_scene_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/scene/*.cpp")
file(GLOB_RECURSE lingze_scene_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/scene/*.h")

# Shader文件
file(GLOB_RECURSE shader_files CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/shaders/glsl/*.frag"
    "${CMAKE_SOURCE_DIR}/shaders/glsl/*.vert"
)
list(APPEND shader_files "${CMAKE_SOURCE_DIR}/shaders/buildShaders.bat")

# 创建引擎库，包含除了应用程序入口点之外的所有代码
set(lingze_lib_sources 
    ${lingze_backend_sources}
    ${lingze_render_common_sources}
    ${lingze_renderers_sources}
    ${lingze_scene_sources}
    ${lingze_application_headers}
    ${lingze_application_sources}
)

set(lingze_lib_headers
    ${lingze_backend_headers}
    ${lingze_render_common_headers}
    ${lingze_renderers_headers}
    ${lingze_scene_headers}
    ${lingze_application_headers}
)

# 创建引擎核心库，供所有应用程序使用
add_library(LingzeEngine STATIC ${lingze_lib_sources} ${lingze_lib_headers})
add_dependencies(LingzeEngine CompileShaders)
target_link_libraries(LingzeEngine ${Vulkan_LIBRARIES} ${GLFW3_LIBRARIES} deps_include ${LINGZE_DEPS_LIB})

# 为源文件创建源码组，以在IDE中更好地组织文件
source_group("Application" FILES ${lingze_application_sources} ${lingze_application_headers})
source_group("Backend" FILES ${lingze_backend_sources} ${lingze_backend_headers})
source_group("Render\\Common" FILES ${lingze_render_common_sources} ${lingze_render_common_headers})
source_group("Render\\Renderers" FILES ${lingze_renderers_sources} ${lingze_renderers_headers})
source_group("Scene" FILES ${lingze_scene_sources} ${lingze_scene_headers})
source_group("Shaders" FILES ${shader_files})

# 自动查找所有App程序
file(GLOB app_files CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/*App.cpp")

# 为每个查找到的App创建可执行文件
foreach(app_file ${app_files})
    # 从文件路径中提取App名称（不含路径和扩展名）
    get_filename_component(app_name ${app_file} NAME_WE)
    
    # 创建可执行文件
    add_executable(${app_name} ${app_file})
    target_link_libraries(${app_name} LingzeEngine)
    
    # 设置输出目录
    set_target_properties(${app_name} PROPERTIES 
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/cmaked"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/cmake"
    )
    
    message(STATUS "Found application: ${app_name}")
endforeach()

# 确保VS2022默认启动LingzeApp（如果存在）
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT LingzeApp)

# TODO: 如有需要，请添加测试并安装目标。
