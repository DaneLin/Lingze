# 依赖库管理

# 设置各种包含目录（仍然保留这些变量，以防其他地方需要使用）
set(TINYOBJ_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tiny_obj_loader/include PARENT_SCOPE)
set(SPIRVCROSS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross PARENT_SCOPE)
set(GLI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gli PARENT_SCOPE)
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm/include PARENT_SCOPE)
set(GLFW3_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include PARENT_SCOPE)
set(JSONCPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp PARENT_SCOPE)
set(IMGUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui PARENT_SCOPE)

# 设置库文件路径
set(GLFW3_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/glfw/lib-vc2015/glfw3.lib" PARENT_SCOPE)

# 创建一个INTERFACE库来管理所有的包含路径
add_library(deps_include INTERFACE)
target_include_directories(deps_include INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/tiny_obj_loader/include
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross
    ${CMAKE_CURRENT_SOURCE_DIR}/gli
    ${CMAKE_CURRENT_SOURCE_DIR}/glm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# 收集依赖库源文件
file(GLOB_RECURSE SPIRV_CROSS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/*.cpp)
file(GLOB_RECURSE JSONCPP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/*.cpp)
file(GLOB_RECURSE IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)

# 构建第三方库
add_library(lingze_deps STATIC 
    ${SPIRV_CROSS_SOURCES}
    ${JSONCPP_SOURCES}
    ${IMGUI_SOURCES}
)

# 为第三方库添加正确的包含路径
target_include_directories(lingze_deps PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross
    ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# 将库导出到父作用域
set(LINGZE_DEPS_LIB lingze_deps PARENT_SCOPE) 