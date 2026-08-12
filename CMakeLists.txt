cmake_minimum_required(VERSION 3.20)
project(VulkanRenderer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ------------------------------------------------------------------
# Vulkan (uses the VULKAN_SDK env var set by the LunarG SDK install
# on both Windows and macOS - no hardcoded SDK path needed)
# ------------------------------------------------------------------
find_package(Vulkan REQUIRED)

# ------------------------------------------------------------------
# GLFW
# ------------------------------------------------------------------
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ------------------------------------------------------------------
# GLM (header-only)
# ------------------------------------------------------------------
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

# ------------------------------------------------------------------
# Dear ImGui (docking branch - required for DockSpaceOverViewport)
# Built as a small static library from the fetched sources.
# ------------------------------------------------------------------
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

# ------------------------------------------------------------------
# Your renderer
# ------------------------------------------------------------------
file(GLOB SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADERS ${CMAKE_SOURCE_DIR}/src/*.h)

add_executable(VulkanRenderer ${SOURCES} ${HEADERS})

add_library(tinyfiledialogs STATIC
    ${CMAKE_SOURCE_DIR}/external/tinyfiledialogs/tinyfiledialogs.h
    ${CMAKE_SOURCE_DIR}/external/tinyfiledialogs/tinyfiledialogs.c
)

target_include_directories(VulkanRenderer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/stb_image
    ${CMAKE_SOURCE_DIR}/external/tinyobjloader
    ${CMAKE_SOURCE_DIR}/external/tinyfiledialogs
)

target_link_libraries(VulkanRenderer PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    imgui
    tinyfiledialogs
)

# Bakes the asset/shader-output directories into the binary at compile
# time, so the source no longer needs any hardcoded absolute paths.
target_compile_definitions(VulkanRenderer PRIVATE
    ASSET_DIR="${CMAKE_SOURCE_DIR}/assets/"
    SHADER_DIR="${CMAKE_BINARY_DIR}/shaders/"
)

# ------------------------------------------------------------------
# Compile shaders at build time with glslc (ships with the Vulkan SDK)
# ------------------------------------------------------------------
find_program(GLSLC_EXECUTABLE glslc HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin")
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found - make sure the Vulkan SDK is installed and VULKAN_SDK is set")
endif()

set(SHADER_MAP
    "shader.vert=vert.spv"
    "shader.frag=frag.spv"
    "imgui_shader.vert=imgui_vert.spv"
    "imgui_shader.frag=imgui_frag.spv"
)

set(SPIRV_BINARIES "")
foreach(ENTRY ${SHADER_MAP})
    string(REPLACE "=" ";" PAIR ${ENTRY})
    list(GET PAIR 0 SRC_NAME)
    list(GET PAIR 1 OUT_NAME)
    set(SRC_PATH "${CMAKE_SOURCE_DIR}/shaders/${SRC_NAME}")
    set(OUT_PATH "${CMAKE_BINARY_DIR}/shaders/${OUT_NAME}")

    add_custom_command(
        OUTPUT ${OUT_PATH}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
        COMMAND ${GLSLC_EXECUTABLE} ${SRC_PATH} -o ${OUT_PATH}
        DEPENDS ${SRC_PATH}
        COMMENT "Compiling shader ${SRC_NAME} -> ${OUT_NAME}"
    )
    list(APPEND SPIRV_BINARIES ${OUT_PATH})
endforeach()

add_custom_target(Shaders DEPENDS ${SPIRV_BINARIES})
add_dependencies(VulkanRenderer Shaders)

# ------------------------------------------------------------------
# macOS / MoltenVK note:
# VulkanContext.cpp/.h need the portability-enumeration instance
# extension and portability-subset device extension enabled under
# __APPLE__. See the accompanying instructions for the exact patch.
# ------------------------------------------------------------------
