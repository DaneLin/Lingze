# CMakeList.txt: CMake project for Lingze, includes source code and defines
# project-specific logic.
#
cmake_minimum_required (VERSION 3.10)

# Enable hot reload for MSVC compiler if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(LingzeEngineCMake VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
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

add_compile_options(/GR)

# Define shader paths for use in code
add_compile_definitions(
    SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/"
    SHADER_GLSL_DIR="${CMAKE_SOURCE_DIR}/shaders/glsl/"
    SHADER_HLSL_DIR="${CMAKE_SOURCE_DIR}/shaders/hlsl/"
    SHADER_SPIRV_GLSL_DIR="${CMAKE_SOURCE_DIR}/shaders/spirv_glsl/"
    SHADER_SPIRV_HLSL_DIR="${CMAKE_SOURCE_DIR}/shaders/spirv_hlsl/"
    SCENE_DIR="${CMAKE_SOURCE_DIR}/data/scenes/"
    DATA_DIR="${CMAKE_SOURCE_DIR}/data/"
    GLTF_DIR="${CMAKE_SOURCE_DIR}/data/glTF-Sample-Assets/Models/"
    # Resolve macro conflicts between GLFW and Windows headers
    GLFW_INCLUDE_NONE
)

# Automatically find all source files using GLOB_RECURSE
# Main directory files (only includes .cpp files in the root directory)
file(GLOB lingze_main_sources CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB lingze_main_headers CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.h")

# Backend files - 直接列出所有文件而不是使用GLOB_RECURSE
set(lingze_backend_sources
    "${CMAKE_SOURCE_DIR}/src/backend/App.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/Core.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/ShaderProgram.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/DescriptorSetCache.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/Buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/StagedResources.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/Sampler.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/Pipeline.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/tiny_loader_impl.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/Camera.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/VertexDeclaration.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/TimestampQuery.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/Swapchain.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/ShaderMemoryPool.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/ShaderModule.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/RenderPass.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/RenderPassCache.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/RenderGraph.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/PresentQueue.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/PipelineCache.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/ImageView.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/Image.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/GpuProfiler.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/Framebuffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/backend/CpuProfiler.cpp"
)

set(lingze_backend_headers
    "${CMAKE_SOURCE_DIR}/src/backend/Logging.h"
    "${CMAKE_SOURCE_DIR}/src/backend/ShaderProgram.h"
    "${CMAKE_SOURCE_DIR}/src/backend/DescriptorSetCache.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Core.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Config.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Buffer.h"
    "${CMAKE_SOURCE_DIR}/src/backend/StagedResources.h"
    "${CMAKE_SOURCE_DIR}/src/backend/ImageLoader.h"
    "${CMAKE_SOURCE_DIR}/src/backend/PipelineCache.h"
    "${CMAKE_SOURCE_DIR}/src/backend/TimestampQuery.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Sampler.h"
    "${CMAKE_SOURCE_DIR}/src/backend/RenderGraph.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Image.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Swapchain.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Synchronization.h"
    "${CMAKE_SOURCE_DIR}/src/backend/ShaderModule.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Pipeline.h"
    "${CMAKE_SOURCE_DIR}/src/backend/RenderPass.h"
    "${CMAKE_SOURCE_DIR}/src/backend/ShaderMemoryPool.h"
    "${CMAKE_SOURCE_DIR}/src/backend/RenderPassCache.h"
    "${CMAKE_SOURCE_DIR}/src/backend/ImageView.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Framebuffer.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Surface.h"
    "${CMAKE_SOURCE_DIR}/src/backend/PresentQueue.h"
    "${CMAKE_SOURCE_DIR}/src/backend/VertexDeclaration.h"
    "${CMAKE_SOURCE_DIR}/src/backend/App.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Camera.h"
    "${CMAKE_SOURCE_DIR}/src/backend/QueueIndices.h"
    "${CMAKE_SOURCE_DIR}/src/backend/ProfilerTask.h"
    "${CMAKE_SOURCE_DIR}/src/backend/GpuProfiler.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Handles.h"
    "${CMAKE_SOURCE_DIR}/src/backend/CpuProfiler.h"
    "${CMAKE_SOURCE_DIR}/src/backend/Pool.h"
)

# Render common files - 直接列出所有文件而不是使用GLOB_RECURSE
set(lingze_render_common_sources
    "${CMAKE_SOURCE_DIR}/src/render/MaterialSystem.cpp"
    "${CMAKE_SOURCE_DIR}/src/render/RenderContext.cpp"
    "${CMAKE_SOURCE_DIR}/src/render/ImguiRenderer.cpp"
    "${CMAKE_SOURCE_DIR}/src/render/MipBuilder.cpp"
    "${CMAKE_SOURCE_DIR}/src/render/ImGuiProfilerRenderer.cpp"
)

set(lingze_render_common_headers
    "${CMAKE_SOURCE_DIR}/src/render/RenderContext.h"
    "${CMAKE_SOURCE_DIR}/src/render/MaterialSystem.h"
    "${CMAKE_SOURCE_DIR}/src/render/ImguiRenderer.h"
    "${CMAKE_SOURCE_DIR}/src/render/BaseRenderer.h"
    "${CMAKE_SOURCE_DIR}/src/render/MipBuilder.h"
    "${CMAKE_SOURCE_DIR}/src/render/ImGuiProfilerRenderer.h"

)

# Scene files - 直接列出所有文件而不是使用GLOB_RECURSE
set(lingze_scene_sources
    "${CMAKE_SOURCE_DIR}/src/scene/MeshLoader.cpp"
    "${CMAKE_SOURCE_DIR}/src/scene/Mesh.cpp"
    "${CMAKE_SOURCE_DIR}/src/scene/Transform.cpp"
    "${CMAKE_SOURCE_DIR}/src/scene/Scene.cpp"
    "${CMAKE_SOURCE_DIR}/src/scene/StaticMeshComponent.cpp"
    "${CMAKE_SOURCE_DIR}/src/scene/Entity.cpp"
    "${CMAKE_SOURCE_DIR}/src/scene/Component.cpp"
    "${CMAKE_SOURCE_DIR}/src/scene/CameraComponent.cpp"
)

set(lingze_scene_headers
    "${CMAKE_SOURCE_DIR}/src/scene/Mesh.h"
    "${CMAKE_SOURCE_DIR}/src/scene/MeshLoader.h"
    "${CMAKE_SOURCE_DIR}/src/scene/Transform.h"
    "${CMAKE_SOURCE_DIR}/src/scene/Scene.h"
    "${CMAKE_SOURCE_DIR}/src/scene/StaticMeshComponent.h"
    "${CMAKE_SOURCE_DIR}/src/scene/Config.h"
    "${CMAKE_SOURCE_DIR}/src/scene/Entity.h"
    "${CMAKE_SOURCE_DIR}/src/scene/CameraComponent.h"
    "${CMAKE_SOURCE_DIR}/src/scene/Component.h"
)

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
target_link_libraries(LingzeEngine ${Vulkan_LIBRARIES} ${GLFW3_LIBRARIES} deps_include ${LINGZE_DEPS_LIB})

# Create source groups to better organize files in the IDE
source_group("Application\\Common" FILES ${lingze_application_common_headers})
source_group("Application\\SimpleTriangle" FILES ${lingze_simpletriangle_sources} ${lingze_simpletriangle_headers})
source_group("Application\\SimpleMeshShading" FILES ${lingze_simplemesh_sources} ${lingze_simplemesh_headers})
source_group("Application\\BasicShape" FILES ${lingze_basicshape_sources} ${lingze_basicshape_headers})
source_group("Backend" FILES ${lingze_backend_sources} ${lingze_backend_headers})
source_group("Render" FILES ${lingze_render_common_sources} ${lingze_render_common_headers})
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
        
        # 获取应用程序目录中的所有源文件
        set(app_source_files "")
        set(app_header_files "")
        
        # 针对每个应用程序目录，手动收集文件
        if(${dir_name} STREQUAL "SimpleTriangle")
            file(GLOB app_source_files "${app_dir}/*.cpp")
            file(GLOB app_header_files "${app_dir}/*.h")
        elseif(${dir_name} STREQUAL "SimpleMeshShading")
            file(GLOB app_source_files "${app_dir}/*.cpp")
            file(GLOB app_header_files "${app_dir}/*.h")
        elseif(${dir_name} STREQUAL "MeshShading")
            file(GLOB app_source_files "${app_dir}/*.cpp")
            file(GLOB app_header_files "${app_dir}/*.h")
        elseif(${dir_name} STREQUAL "GpuDriven")
            file(GLOB app_source_files "${app_dir}/*.cpp")
            file(GLOB app_header_files "${app_dir}/*.h")
        endif()
        
        # 保留原始逻辑，检查是否有源文件
        if(app_source_files)
            # Find a suitable main file - look for patterns that could indicate an entry point
            set(main_file "")
            
            # First, try to find a file that contains "main" or "App" in its name
            foreach(src_file ${app_source_files})
                get_filename_component(file_name ${src_file} NAME)
                string(TOLOWER ${file_name} file_name_lower)
                
                if(file_name_lower MATCHES "main" OR file_name_lower MATCHES "app")
                    set(main_file ${src_file})
                    break()
                endif()
            endforeach()
            
            # If no main file found, use the first cpp file
            if(NOT main_file AND app_source_files)
                list(GET app_source_files 0 main_file)
            endif()
            
            # If we found a main file, create an executable
            if(main_file)
                # Use the directory name as the application name
                set(app_name "${dir_name}App")
                
                # Create executable with all source files in the directory
                add_executable(${app_name} ${main_file} ${app_source_files} ${app_header_files})
                target_link_libraries(${app_name} LingzeEngine)
                
                # Set output directory
                set_target_properties(${app_name} PROPERTIES 
                    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/cmaked"
                    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/cmake"
                )
                
                message(STATUS "Found application: ${app_name} (main file: ${main_file})")
                
                # Add to our list of found applications
                list(APPEND found_applications ${app_name})
            endif()
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
