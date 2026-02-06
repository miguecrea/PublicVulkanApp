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
# Shader tools
# -------------------------------------------------
find_program(GLSLANG_VALIDATOR glslangValidator
    HINTS $ENV{VULKAN_SDK}/Bin
    REQUIRED
)

add_executable(glslang_validator IMPORTED GLOBAL)
set_target_properties(glslang_validator PROPERTIES
    IMPORTED_LOCATION "${GLSLANG_VALIDATOR}"
)

# -------------------------------------------------
# Shader helper
# -------------------------------------------------
function(add_shaders_target TARGET)
    cmake_parse_arguments(SHADER "" "CHAPTER_NAME" "SOURCES" ${ARGN})

    set(SHADERS_DIR ${CMAKE_BINARY_DIR}/${SHADER_CHAPTER_NAME}/shaders)

    add_custom_command(
        OUTPUT ${SHADERS_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADERS_DIR}
    )

    add_custom_command(
        OUTPUT ${SHADERS_DIR}/vert.spv ${SHADERS_DIR}/frag.spv
        COMMAND glslang_validator
        ARGS --target-env vulkan1.0 ${SHADER_SOURCES} --quiet
        WORKING_DIRECTORY ${SHADERS_DIR}
        DEPENDS ${SHADER_SOURCES}
        COMMENT "Compiling shaders"
        VERBATIM
    )

    add_custom_target(${TARGET}
        DEPENDS ${SHADERS_DIR}/vert.spv ${SHADERS_DIR}/frag.spv
    )
endfunction()

# -------------------------------------------------
# Chapter helper
# -------------------------------------------------
function(add_chapter CHAPTER_NAME)
    cmake_parse_arguments(CHAPTER "" "SHADER" "LIBS;TEXTURES;MODELS" ${ARGN})

    add_executable(${CHAPTER_NAME} ${CHAPTER_NAME}.cpp)

    set_target_properties(${CHAPTER_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CHAPTER_NAME}
    )

    target_link_libraries(${CHAPTER_NAME}
        PRIVATE
            Vulkan::cppm
            glfw
            glm::glm
            tinyobjloader::tinyobjloader
              KTX::ktx
    )

    target_include_directories(${CHAPTER_NAME}
        PRIVATE
            ${TINYGLTF_INCLUDE_DIR}
            ${STB_INCLUDE_DIR}
    )

    target_compile_definitions(${CHAPTER_NAME}
        PRIVATE VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
    )

    if(WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio")
        set_target_properties(${CHAPTER_NAME} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY
            "${CMAKE_BINARY_DIR}/${CHAPTER_NAME}"
        )
    endif()
endfunction()

# -------------------------------------------------
# Executable
# -------------------------------------------------
add_chapter(Main)
