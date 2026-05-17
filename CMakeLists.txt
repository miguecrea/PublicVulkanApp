cmake_minimum_required(VERSION 3.20)
project(VulkanRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------
# Dependencies via FetchContent (no vcpkg needed)
# -------------------------------------------------------
include(FetchContent)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

# GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.9
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# STB
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

# tinyobjloader
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        v1.0.6
)
FetchContent_MakeAvailable(tinyobjloader)

# tinygltf
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.8.21
)
FetchContent_MakeAvailable(tinygltf)


# -------------------------------------------------------
# Heavy assets (Sponza model + textures + HDR, ~2.8 GB)
# Hosted at https://github.com/miguecrea/Assets as Release attachments.
# Auto-downloaded at configure time if missing.
# -------------------------------------------------------
option(FETCH_ASSETS "Auto-download heavy assets from miguecrea/Assets if missing" ON)

set(ASSETS_MARKER "${CMAKE_SOURCE_DIR}/Models/Sponza/NewSponza_Main_glTF_003.bin")

if(FETCH_ASSETS AND NOT EXISTS "${ASSETS_MARKER}")
    set(ASSETS_BASE_URL "https://github.com/miguecrea/Assets/releases/latest/download")
    set(ASSETS_ZIPS mesh.zip env.zip textures_part1.zip textures_part2.zip)
    set(ASSETS_TMP "${CMAKE_BINARY_DIR}/_assets_dl")
    file(MAKE_DIRECTORY "${ASSETS_TMP}")

    message(STATUS "Heavy assets missing - fetching from ${ASSETS_BASE_URL}")
    message(STATUS "One-time ~2.8 GB download (skip with -DFETCH_ASSETS=OFF)")

    foreach(zip IN LISTS ASSETS_ZIPS)
        set(zip_path "${ASSETS_TMP}/${zip}")
        message(STATUS "  Downloading ${zip}...")
        file(DOWNLOAD
            "${ASSETS_BASE_URL}/${zip}"
            "${zip_path}"
            SHOW_PROGRESS
            STATUS dl_status
        )
        list(GET dl_status 0 dl_code)
        if(NOT dl_code EQUAL 0)
            list(GET dl_status 1 dl_msg)
            file(REMOVE "${zip_path}")
            message(FATAL_ERROR "Failed to download ${zip}: ${dl_msg}")
        endif()
        message(STATUS "  Extracting ${zip}...")
        file(ARCHIVE_EXTRACT
            INPUT "${zip_path}"
            DESTINATION "${CMAKE_SOURCE_DIR}"
        )
        file(REMOVE "${zip_path}")
    endforeach()

    file(REMOVE_RECURSE "${ASSETS_TMP}")
    message(STATUS "Assets ready")
endif()

# -------------------------------------------------------
# Vulkan SDK (user must have it installed)
# -------------------------------------------------------
find_package(Vulkan REQUIRED)

# -------------------------------------------------------
# Shader compilation
# -------------------------------------------------------
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin)

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/ShadersOutput)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

file(GLOB VERT_SHADERS "${SHADER_SOURCE_DIR}/*.vert")
file(GLOB FRAG_SHADERS "${SHADER_SOURCE_DIR}/*.frag")

set(COMPILED_SHADERS)
foreach(SHADER ${VERT_SHADERS} ${FRAG_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${GLSLC} ${SHADER} -o ${OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER_NAME}"
    )
    list(APPEND COMPILED_SHADERS ${OUTPUT})
endforeach()

file(GLOB COMP_SHADERS "${SHADER_SOURCE_DIR}/*.comp")
foreach(SHADER ${COMP_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${GLSLC} ${SHADER} -o ${OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling compute shader: ${SHADER_NAME}"
    )
    list(APPEND COMPILED_SHADERS ${OUTPUT})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${COMPILED_SHADERS})

# -------------------------------------------------------
# Source files
# -------------------------------------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})
add_dependencies(${PROJECT_NAME} Shaders)

# -------------------------------------------------------
# Include paths
# -------------------------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${stb_SOURCE_DIR}
    ${tinyobjloader_SOURCE_DIR}
    ${tinygltf_SOURCE_DIR}   
)

# -------------------------------------------------------
# Link libraries
# -------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    tinyobjloader
)

# -------------------------------------------------------
# Project source dir define (for loading assets at runtime)
# -------------------------------------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
