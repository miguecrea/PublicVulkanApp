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
# Vulkan SDK (user must have it installed)
# -------------------------------------------------------
find_package(Vulkan REQUIRED)

# -------------------------------------------------------
# Shader compilation
# -------------------------------------------------------
find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/Bin)

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Shaders)
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
