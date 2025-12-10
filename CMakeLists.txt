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
    # Enable universal binary for Apple Silicon + Intel (optional)
    # set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    set(CMAKE_OSX_ARCHITECTURES "arm64")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
# Vulkan (via MoltenVK on macOS)
find_package(Vulkan REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${Vulkan_INCLUDE_DIRS}
)

# Source files (will be populated as development progresses)
set(ENGINE_SOURCES
    src/main.cpp
)

# Renderer module
set(RENDERER_SOURCES
    # src/renderer/vulkan_context.cpp
    # src/renderer/deferred_pipeline.cpp
    # src/renderer/shadow_mapping.cpp
    # src/renderer/material.cpp
    # src/renderer/mesh.cpp
)

# Physics module
set(PHYSICS_SOURCES
    # src/physics/physics_world.cpp
    # src/physics/character_controller.cpp
    # src/physics/ragdoll.cpp
)

# Animation module
set(ANIMATION_SOURCES
    # src/animation/skeleton.cpp
    # src/animation/animation_clip.cpp
    # src/animation/animation_state_machine.cpp
    # src/animation/ik_solver.cpp
)

# Particles module
set(PARTICLES_SOURCES
    # src/particles/particle_system.cpp
    # src/particles/emitter.cpp
)

# Audio module
set(AUDIO_SOURCES
    # src/audio/audio_engine.cpp
    # src/audio/synthesizer.cpp
)

# Network module
set(NETWORK_SOURCES
    # src/network/network_manager.cpp
    # src/network/client.cpp
    # src/network/server.cpp
)

# Game module
set(GAME_SOURCES
    # src/game/player.cpp
    # src/game/weapon.cpp
    # src/game/game_mode.cpp
    # src/game/map_generator.cpp
)

# Input module
set(INPUT_SOURCES
    # src/input/input_manager.cpp
)

# Create executable
add_executable(${PROJECT_NAME}
    ${ENGINE_SOURCES}
    ${RENDERER_SOURCES}
    ${PHYSICS_SOURCES}
    ${ANIMATION_SOURCES}
    ${PARTICLES_SOURCES}
    ${AUDIO_SOURCES}
    ${NETWORK_SOURCES}
    ${GAME_SOURCES}
    ${INPUT_SOURCES}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${Vulkan_LIBRARIES}
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

# External libraries (to be added)
# find_package(assimp REQUIRED)
# find_package(enet REQUIRED)
# find_package(bullet REQUIRED)

# Shader compilation (GLSL to SPIR-V)
# find_program(GLSLC glslc)
# if(GLSLC)
#     file(GLOB_RECURSE SHADER_SOURCES
#         "${CMAKE_SOURCE_DIR}/assets/shaders/*.vert"
#         "${CMAKE_SOURCE_DIR}/assets/shaders/*.frag"
#         "${CMAKE_SOURCE_DIR}/assets/shaders/*.comp"
#     )
#     foreach(SHADER ${SHADER_SOURCES})
#         get_filename_component(SHADER_NAME ${SHADER} NAME)
#         set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
#         add_custom_command(
#             OUTPUT ${SHADER_OUTPUT}
#             COMMAND ${GLSLC} ${SHADER} -o ${SHADER_OUTPUT}
#             DEPENDS ${SHADER}
#         )
#         list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
#     endforeach()
#     add_custom_target(shaders DEPENDS ${SHADER_OUTPUTS})
#     add_dependencies(${PROJECT_NAME} shaders)
# endif()

# Enable testing
enable_testing()
add_subdirectory(tests EXCLUDE_FROM_ALL)

# Install rules
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY assets/ DESTINATION share/${PROJECT_NAME}/assets)
