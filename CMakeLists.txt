cmake_minimum_required(VERSION 3.20)
project(SlamEngine VERSION 0.1.0 LANGUAGES CXX)

# C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# macOS/Apple Silicon specific settings
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${Vulkan_INCLUDE_DIRS}
)

# Source files
set(ENGINE_SOURCES
    src/main.cpp
)

# Renderer module
set(RENDERER_SOURCES
    src/renderer/vulkan_context.cpp
    src/renderer/pipeline.cpp
    src/renderer/mesh.cpp
    src/renderer/camera.cpp
)

# Input module
set(INPUT_SOURCES
    src/input/window.cpp
    src/input/input_manager.cpp
)

# Physics module (placeholder)
set(PHYSICS_SOURCES
)

# Animation module (placeholder)
set(ANIMATION_SOURCES
)

# Particles module (placeholder)
set(PARTICLES_SOURCES
)

# Audio module (placeholder)
set(AUDIO_SOURCES
)

# Network module (placeholder)
set(NETWORK_SOURCES
)

# Game module (placeholder)
set(GAME_SOURCES
)

# Create executable
add_executable(${PROJECT_NAME}
    ${ENGINE_SOURCES}
    ${RENDERER_SOURCES}
    ${INPUT_SOURCES}
    ${PHYSICS_SOURCES}
    ${ANIMATION_SOURCES}
    ${PARTICLES_SOURCES}
    ${AUDIO_SOURCES}
    ${NETWORK_SOURCES}
    ${GAME_SOURCES}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${Vulkan_LIBRARIES}
    glfw
)

# macOS frameworks
if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        "-framework Cocoa"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework IOKit"
        "-framework CoreAudio"
        "-framework AudioToolbox"
    )
endif()

# Shader compilation (GLSL to SPIR-V)
find_program(GLSLC glslc)
if(GLSLC)
    message(STATUS "Found glslc: ${GLSLC}")

    # Create shader output directory
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/shaders)

    # Find all shader files
    file(GLOB_RECURSE SHADER_SOURCES
        "${CMAKE_SOURCE_DIR}/assets/shaders/*.vert"
        "${CMAKE_SOURCE_DIR}/assets/shaders/*.frag"
        "${CMAKE_SOURCE_DIR}/assets/shaders/*.comp"
    )

    # Compile each shader
    foreach(SHADER ${SHADER_SOURCES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
        add_custom_command(
            OUTPUT ${SHADER_OUTPUT}
            COMMAND ${GLSLC} ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling shader ${SHADER_NAME}"
        )
        list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    endforeach()

    # Add shader target
    add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})
    add_dependencies(${PROJECT_NAME} shaders)
else()
    message(WARNING "glslc not found - shaders will not be compiled")
endif()

# Copy shaders to binary directory for runtime access
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_BINARY_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

# Enable testing
enable_testing()
add_subdirectory(tests EXCLUDE_FROM_ALL)

# Install rules
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY assets/ DESTINATION share/${PROJECT_NAME}/assets)
install(DIRECTORY ${CMAKE_BINARY_DIR}/shaders/ DESTINATION share/${PROJECT_NAME}/shaders)
