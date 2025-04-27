# CMakeList.txt: CMake project for Lingze, includes source code and defines
# project-specific logic.
#
cmake_minimum_required (VERSION 3.8)

# Enable hot reload for MSVC compiler if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(LingzeEngineCMake VERSION 0.1.0)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

# Enable VS folders and filters
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Configure VS CMake settings to automatically update when files are added
if(MSVC)
  set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD 1)
  set(CMAKE_SUPPRESS_REGENERATION OFF)
endif()

find_package(Vulkan)
message(STATUS "Found Vulkan in ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Found Vulkan libs in ${Vulkan_LIBRARIES}")

# Add deps subdirectory, dependency configurations are managed in deps/CMakeLists.txt
add_subdirectory(deps)

# Add project include paths
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${Vulkan_INCLUDE_DIRS}")


# Define shader paths for use in code
add_compile_definitions(
    SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/"
    SHADER_SPIRV_DIR="${CMAKE_SOURCE_DIR}/shaders/spirv/"
    SHADER_GLSL_DIR="${CMAKE_SOURCE_DIR}/shaders/glsl/"
    SCENE_DIR="${CMAKE_SOURCE_DIR}/data/scenes/"
    # Resolve macro conflicts between GLFW and Windows headers
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

# Automatically find all source files using GLOB_RECURSE
# Main directory files (only includes .cpp files in the root directory)
file(GLOB lingze_main_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB lingze_main_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.h")

# Application common files
file(GLOB lingze_application_common_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/*.h")

# Application demo projects
# SimpleTriangle files
file(GLOB_RECURSE lingze_simpletriangle_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/SimpleTriangle/*.cpp")
file(GLOB_RECURSE lingze_simpletriangle_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/SimpleTriangle/*.h")

# SimpleMeshShading files
file(GLOB_RECURSE lingze_simplemesh_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/SimpleMeshShading/*.cpp")
file(GLOB_RECURSE lingze_simplemesh_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/SimpleMeshShading/*.h")

# BasicShape files
file(GLOB_RECURSE lingze_basicshape_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/BasicShape/*.cpp")
file(GLOB_RECURSE lingze_basicshape_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/BasicShape/*.h")

# Backend files
file(GLOB_RECURSE lingze_backend_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/backend/*.cpp")
file(GLOB_RECURSE lingze_backend_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/backend/*.h")

# Render common files
file(GLOB_RECURSE lingze_render_common_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/render/common/*.cpp")
file(GLOB_RECURSE lingze_render_common_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/render/common/*.h")

# Renderer files
file(GLOB_RECURSE lingze_renderers_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/render/renderers/*.cpp")
file(GLOB_RECURSE lingze_renderers_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/render/renderers/*.h")

# Scene files
file(GLOB_RECURSE lingze_scene_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/scene/*.cpp")
file(GLOB_RECURSE lingze_scene_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/scene/*.h")

# Create engine library, including all code except the application entry point
set(lingze_lib_sources 
    ${lingze_backend_sources}
    ${lingze_render_common_sources}
    ${lingze_renderers_sources}
    ${lingze_scene_sources}
    ${lingze_simpletriangle_sources}
    ${lingze_simplemesh_sources}
    ${lingze_basicshape_sources}
)

set(lingze_lib_headers
    ${lingze_backend_headers}
    ${lingze_render_common_headers}
    ${lingze_renderers_headers}
    ${lingze_scene_headers}
    ${lingze_application_common_headers}
    ${lingze_simpletriangle_headers}
    ${lingze_simplemesh_headers}
    ${lingze_basicshape_headers}
)

# Create engine core library for all applications to use
add_library(LingzeEngine STATIC ${lingze_lib_sources} ${lingze_lib_headers})
add_dependencies(LingzeEngine CompileShaders)
target_link_libraries(LingzeEngine ${Vulkan_LIBRARIES} ${GLFW3_LIBRARIES} deps_include ${LINGZE_DEPS_LIB})

# Create source groups to better organize files in the IDE
source_group("Application\\Common" FILES ${lingze_application_common_headers})
source_group("Application\\SimpleTriangle" FILES ${lingze_simpletriangle_sources} ${lingze_simpletriangle_headers})
source_group("Application\\SimpleMeshShading" FILES ${lingze_simplemesh_sources} ${lingze_simplemesh_headers})
source_group("Application\\BasicShape" FILES ${lingze_basicshape_sources} ${lingze_basicshape_headers})
source_group("Backend" FILES ${lingze_backend_sources} ${lingze_backend_headers})
source_group("Render\\Common" FILES ${lingze_render_common_sources} ${lingze_render_common_headers})
source_group("Render\\Renderers" FILES ${lingze_renderers_sources} ${lingze_renderers_headers})
source_group("Scene" FILES ${lingze_scene_sources} ${lingze_scene_headers})
source_group("Shaders" FILES ${shader_files})

# Get list of all application directories
file(GLOB app_dirs CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/application/*")

# Variables to store successful application names
set(found_applications "")

# Iterate through each application directory
foreach(app_dir ${app_dirs})
    if(IS_DIRECTORY ${app_dir})
        # Get the directory name
        get_filename_component(dir_name ${app_dir} NAME)
        
        # Skip if this is not a real application directory (e.g., if it's just a common header file)
        if(EXISTS "${app_dir}/${dir_name}App.cpp" OR EXISTS "${app_dir}/${dir_name}App.h")
            # Find the app file
            file(GLOB app_files CONFIGURE_DEPENDS "${app_dir}/*App.cpp")
            
            # Create an executable for each app found
            foreach(app_file ${app_files})
                # Extract app name from file path (without path and extension)
                get_filename_component(app_name ${app_file} NAME_WE)
                
                # Create executable
                add_executable(${app_name} ${app_file})
                target_link_libraries(${app_name} LingzeEngine)
                
                # Set output directory
                set_target_properties(${app_name} PROPERTIES 
                    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/cmaked"
                    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/cmake"
                )
                
                message(STATUS "Found application: ${app_name}")
                
                # Add to our list of found applications
                list(APPEND found_applications ${app_name})
            endforeach()
        endif()
    endif()
endforeach()

# Ensure VS2022 defaults to starting SimpleTriangleApp (if it exists)
if(TARGET SimpleTriangleApp)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SimpleTriangleApp)
# Otherwise use the first application found
elseif(found_applications)
    list(GET found_applications 0 first_app)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${first_app})
endif()

# TODO: Add tests and installation targets if needed.
