cmake_minimum_required(VERSION 3.29)

project(VulkanTutorial LANGUAGES CXX)

# -------------------------------------------------
# Global settings
# -------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# -------------------------------------------------
# Dependencies (vcpkg)
# -------------------------------------------------
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(ktx CONFIG REQUIRED)

find_path(TINYGLTF_INCLUDE_DIR tiny_gltf.h REQUIRED)
find_path(STB_INCLUDE_DIR stb_image.h REQUIRED)

# -------------------------------------------------
# Vulkan-Hpp interface target
# -------------------------------------------------
add_library(VulkanCppModule INTERFACE)
add_library(Vulkan::cppm ALIAS VulkanCppModule)

target_link_libraries(VulkanCppModule
    INTERFACE Vulkan::Vulkan
)

target_compile_definitions(VulkanCppModule
    INTERFACE
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
)

# -------------------------------------------------
# Shader Compiler
# -------------------------------------------------
find_program(GLSLANG_VALIDATOR glslangValidator
    HINTS $ENV{VULKAN_SDK}/Bin
    REQUIRED
)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SPIRV_DIR ${CMAKE_BINARY_DIR}/shaders)

file(MAKE_DIRECTORY ${SPIRV_DIR})

file(GLOB SHADER_SOURCES
    ${SHADER_DIR}/*.vert
    ${SHADER_DIR}/*.frag
)

set(SPIRV_OUTPUTS)

foreach(SHADER ${SHADER_SOURCES})

    get_filename_component(FILE_NAME ${SHADER} NAME)
    set(SPIRV ${SPIRV_DIR}/${FILE_NAME}.spv)

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSLANG_VALIDATOR}
                -V ${SHADER}
                -o ${SPIRV}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${FILE_NAME}"
        VERBATIM
    )

    list(APPEND SPIRV_OUTPUTS ${SPIRV})

endforeach()

add_custom_target(Shaders ALL
    DEPENDS ${SPIRV_OUTPUTS}
)

# -------------------------------------------------
# Source Files
# -------------------------------------------------
file(GLOB_RECURSE APP_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
)

add_executable(VulkanTutorial ${APP_SOURCES}   "Headers/InstanceManager.h" "src/InstanceManager.cpp"  "src/Window.cpp" "Headers/Window.h" "src/VulkanApp.cpp" "Headers/VulkanApp.h"  "src/DeviceManager.cpp" "Headers/DeviceManager.h" "src/SwapChain.cpp" "Headers/SwapChain.h" "Headers/SwapChainSupportDetails.h")

set_target_properties(VulkanTutorial PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# -------------------------------------------------
# Link libraries
# -------------------------------------------------
target_link_libraries(VulkanTutorial
    PRIVATE
        Vulkan::cppm
        glfw
        glm::glm
        tinyobjloader::tinyobjloader
        KTX::ktx
)

target_include_directories(VulkanTutorial
    PRIVATE
        ${TINYGLTF_INCLUDE_DIR}
        ${STB_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(VulkanTutorial
    PRIVATE
        VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
)

# Make executable depend on shaders
add_dependencies(VulkanTutorial Shaders)

# -------------------------------------------------
# Visual Studio debugger working directory
# -------------------------------------------------
if(WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio")
    set_target_properties(VulkanTutorial PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()