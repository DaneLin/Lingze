# Dependency management

# Set various include directories (still keep these variables in case they're needed elsewhere)
set(TINYOBJ_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tiny_obj_loader/include PARENT_SCOPE)
set(SPIRVCROSS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross PARENT_SCOPE)
set(GLI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gli PARENT_SCOPE)
set(GLM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glm/include PARENT_SCOPE)
set(GLFW3_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include PARENT_SCOPE)
set(JSONCPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp PARENT_SCOPE)
set(IMGUI_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui PARENT_SCOPE)

# Set library file paths
set(GLFW3_LIBRARIES "${CMAKE_CURRENT_SOURCE_DIR}/glfw/lib-vc2015/glfw3.lib" PARENT_SCOPE)

# Create an INTERFACE library to manage all include paths
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

# Collect dependency library source files
file(GLOB_RECURSE SPIRV_CROSS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross/*.cpp)
file(GLOB_RECURSE JSONCPP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/*.cpp)
file(GLOB_RECURSE IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp)

# Build third-party libraries
add_library(lingze_deps STATIC 
    ${SPIRV_CROSS_SOURCES}
    ${JSONCPP_SOURCES}
    ${IMGUI_SOURCES}
)

# Add correct include paths for third-party libraries
target_include_directories(lingze_deps PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/spirv-cross
    ${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# Export the library to parent scope
set(LINGZE_DEPS_LIB lingze_deps PARENT_SCOPE) 