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
# Heavy assets (Sponza model + textures + HDR, ~60 MB)
# Hosted at https://github.com/miguecrea/Assets as a Release attachment.
# Downloaded with 3 retries; if the post-extract marker is missing the
# next configure will re-fetch automatically.
# -------------------------------------------------------
option(FETCH_ASSETS "Auto-download heavy assets from miguecrea/Assets if missing" ON)

if(FETCH_ASSETS)
    set(ASSETS_URL    "https://github.com/miguecrea/Assets/releases/latest/download/assets.zip")
    set(ASSETS_MARKER "${CMAKE_SOURCE_DIR}/Models/Sponza/Sponza.bin")
    set(ASSETS_TMP    "${CMAKE_BINARY_DIR}/_assets_dl")

    if(NOT EXISTS "${ASSETS_MARKER}")
        message(STATUS "Heavy assets missing - fetching from ${ASSETS_URL}")
        message(STATUS "(skip with -DFETCH_ASSETS=OFF)")

        file(MAKE_DIRECTORY "${ASSETS_TMP}")
        set(zip_path "${ASSETS_TMP}/assets.zip")

        set(_download_ok FALSE)
        foreach(attempt RANGE 1 3)
            message(STATUS "Downloading assets.zip (attempt ${attempt}/3)")
            file(DOWNLOAD
                "${ASSETS_URL}"
                "${zip_path}"
                SHOW_PROGRESS
                STATUS dl_status
            )
            list(GET dl_status 0 dl_code)
            if(dl_code EQUAL 0)
                set(_download_ok TRUE)
                break()
            endif()
            list(GET dl_status 1 dl_msg)
            message(WARNING "  attempt ${attempt} failed: ${dl_msg}")
            file(REMOVE "${zip_path}")
        endforeach()

        if(NOT _download_ok)
            message(FATAL_ERROR
                "Failed to download assets.zip after 3 attempts. "
                "Re-run cmake, or run scripts/fetch_assets.ps1 -Force manually, "
                "or set -DFETCH_ASSETS=OFF and provide assets yourself."
            )
        endif()

        message(STATUS "Extracting assets.zip")
        file(ARCHIVE_EXTRACT
            INPUT "${zip_path}"
            DESTINATION "${CMAKE_SOURCE_DIR}"
        )
        file(REMOVE_RECURSE "${ASSETS_TMP}")

        if(NOT EXISTS "${ASSETS_MARKER}")
            message(FATAL_ERROR
                "Extraction of assets.zip did not produce Models/Sponza/Sponza.bin - zip may be corrupt. "
                "Try re-running cmake, or scripts/fetch_assets.ps1 -Force."
            )
        endif()
        message(STATUS "Assets ready")
    endif()
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
