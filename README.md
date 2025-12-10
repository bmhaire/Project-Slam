# Slam Engine

A bespoke 4-player networked deathmatch game engine built from scratch.

## Target Platform

- **OS:** macOS (M-series Apple Silicon)
- **Performance:** 60fps at 1440p on M2 MacBook Air
- **Graphics API:** Vulkan (via MoltenVK)

## Features (Planned)

- **Rendering:** Vulkan deferred renderer with PBR materials, clustered lighting, shadow mapping
- **Physics:** Rigid body dynamics, character controller, full ragdoll system
- **Animation:** Skeletal animation, blend trees, IK solvers
- **Particles:** GPU-accelerated particle system (100k+ particles)
- **Audio:** Procedural sound synthesis, 3D positional audio
- **Networking:** Client-server architecture, client-side prediction, lag compensation
- **Procedural Content:** Map generation, material generation

## Project Structure

```
SlamEngine/
├── src/
│   ├── renderer/     # Vulkan rendering pipeline
│   ├── physics/      # Physics engine and collision
│   ├── animation/    # Skeletal animation system
│   ├── particles/    # GPU particle system
│   ├── audio/        # Procedural audio engine
│   ├── network/      # Multiplayer networking
│   ├── game/         # Game logic and rules
│   ├── input/        # Input handling
│   └── utils/        # Math, random, utilities
├── assets/
│   ├── materials/    # Generated PBR textures
│   ├── models/       # FBX character/prop models
│   └── shaders/      # GLSL shader source
├── tools/
│   ├── material_baker/   # Procedural texture generator
│   └── map_viewer/       # Map preview tool
├── external/         # Third-party libraries
└── tests/            # Unit and integration tests
```

## Dependencies

- **Vulkan SDK** (with MoltenVK for macOS)
- **Assimp** - FBX model importing
- **ENet** - UDP networking
- **Bullet Physics** or **Jolt Physics** - Physics simulation

## Building

### Prerequisites

1. Install Vulkan SDK: https://vulkan.lunarg.com/sdk/home
2. Install CMake (3.20+)
3. Install dependencies via Homebrew:
   ```bash
   brew install assimp enet
   ```

### Build Commands

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Run
./build/bin/SlamEngine
```

## Usage

```bash
# Host a game
./SlamEngine --host --seed 12345

# Join a game
./SlamEngine --connect 192.168.1.100

# Controls (default)
WASD        - Move
Mouse       - Look
Space       - Jump
Left Click  - Shoot
R           - Reload
Tab         - Scoreboard
Esc         - Menu
```

## Development Phases

1. **Foundation** - Vulkan setup, basic rendering
2. **Materials** - Procedural PBR texture generation
3. **Lighting** - Deferred rendering, shadows, clustered lights
4. **Maps** - Procedural level generation
5. **Physics** - Character controller, ragdolls
6. **Animation** - Skeletal animation, IK
7. **Particles** - GPU particle system
8. **Audio** - Procedural sound synthesis
9. **Networking** - Multiplayer implementation
10. **Gameplay** - Combat, scoring, match logic
11. **Polish** - Optimization, bug fixes

## License

Private project - All rights reserved
