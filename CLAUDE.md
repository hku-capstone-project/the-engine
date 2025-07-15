# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Commands

### Build System

- **Development build**: `build.bat` (builds debug version by default)
- **Release build**: `build.bat --release`
- **Clean build**: Delete `build/` directory and run `build.bat`

### Build Process

The build system performs these steps:

1. Publishes C# game code to `build/Game/` using `dotnet publish`
2. Configures CMake with vcpkg dependencies
3. Builds C++ engine using Ninja
4. Copies .NET runtime DLLs to executable directory
5. Automatically runs the application

### Dependencies

- **C++ Dependencies**: Managed by vcpkg (see `vcpkg.json`)
- **C# Dependencies**: .NET 8.0 runtime (located in `dep/dotnet-runtime-8.0.16/`)
- **Build Tools**: CMake 3.x, Ninja, Clang 18.1.8, Vulkan SDK

### Game Variants

- **GameScript**: `build.bat GameScript` - Main 3D vampire survivor game
- **Benchmark**: `build.bat Benchmark` - Performance benchmarking mode

## Architecture Overview

### Hybrid C++/C# Engine

This is a data-oriented ECS game engine that combines:

- **C++ Core**: High-performance Vulkan rendering, ECS registry (EnTT), system management
- **C# Scripting**: Game logic, ECS systems, component management via interop
- **Cross-Language Bridge**: P/Invoke-based runtime bridge for seamless integration

### Key Components

#### Entity Component System (ECS)

- **Entity**: 32-bit integer identifier
- **Components**: Plain data structures shared between C++ and C#
- **Systems**: C# methods with `[UpdateSystem]` and `[Query]` attributes
- **Registry**: EnTT-based storage in C++ with C# query interface

#### Cross-Language Integration

- **Runtime Bridge**: `src/dotnet/RuntimeBridge.cpp` - Hosts .NET runtime and provides C++ → C# interop
- **Engine Bindings**: `game/EngineBinding.cs` - C# → C++ function pointers via P/Invoke
- **Component Sync**: Shared component definitions between `src/dotnet/Components.hpp` and `game/Components.cs`

#### Rendering Pipeline

- **Vulkan-based**: Modern graphics API with multi-frame pipelining
- **PBR Materials**: Physically-based rendering with metallic/roughness workflow
- **Asset Loading**: Support for .obj and .gltf models via Assimp
- **Shader System**: GLSL shaders with includes (`resources/shaders/`)

## Project Structure

```plaintext
src/                           # C++ Engine Core
├── app/                      # Application lifecycle and main loop
├── renderer/                 # Vulkan rendering system
├── dotnet/                   # .NET runtime hosting and component definitions
│   ├── RuntimeBridge.cpp     # Main interop bridge
│   ├── Components.hpp        # C++ component structures
│   └── RuntimeApplication.hpp # .NET runtime management
├── window/                   # GLFW window and input handling
├── utils/                    # Utilities (logging, file I/O, Vulkan wrappers)
└── config/                   # Configuration management

game/                          # C# Game Scripts
├── GameScript.cs             # Main game logic and ECS systems
├── EngineBinding.cs          # C++ function bindings
├── Components.cs             # C# component mirrors
└── PluginBootstrap.cs        # System registration

resources/                     # Game Assets
├── models/                   # 3D models (.obj, .gltf)
├── shaders/                  # GLSL shaders with includes
└── configs/                  # Configuration files
```

## Development Workflow

### Adding New Components

1. **Define in C++**: Add struct to `src/dotnet/Components.hpp`
2. **Register in Bridge**: Add component handlers in `src/dotnet/RuntimeBridge.cpp`
3. **Create C# Binding**: Add delegate and function binding in `game/EngineBinding.cs`
4. **Mirror in C#**: Add identical struct to `game/Components.cs`

### Creating ECS Systems

```csharp
[UpdateSystem]
[Query(typeof(Transform), typeof(Velocity))]
public static void MovementSystem(float dt, ref Transform transform, ref Velocity velocity)
{
    transform.position += velocity.velocity * dt;
}
```

### Asset Integration

- Place models in `resources/models/`
- Register via `EngineBindings.RegisterMesh(modelId, "path/to/model.gltf")`
- Create entities with `CreateEntity()` and `AddMesh()`

## Key Files to Understand

### Core Engine Files

- `src/app/Application.cpp` - Main application loop and initialization
- `src/renderer/Renderer.cpp` - Vulkan rendering pipeline
- `src/dotnet/RuntimeBridge.cpp` - C++/C# interop bridge
- `src/window/Window.cpp` - GLFW window and input handling

### Game Logic Files

- `game/GameScript.cs` - Main game systems and logic
- `game/EngineBinding.cs` - C++ function bindings
- `game/Components.cs` - ECS component definitions

### Build Configuration

- `CMakeLists.txt` - Main build configuration
- `build.bat` - Build script with .NET publishing
- `vcpkg.json` - C++ dependency management

## Performance Considerations

### ECS Performance

- Components stored in contiguous arrays for cache efficiency
- Systems process entities in batches for optimal memory access
- Query attributes enable efficient component filtering

### Cross-Language Calls

- Function pointers cached to avoid dynamic lookup
- Bulk operations preferred over individual calls
- Shared memory used for large data structures

### Rendering Optimization

- Multi-frame pipelining reduces GPU idle time
- Descriptor set caching minimizes state changes
- VMA allocator for optimal memory management

## Debugging and Testing

### Logging System

- C# logging via custom `Log()` function
- Automatic log file creation in `logs/` directory
- Timestamped entries for debugging

### Development Tools

- Mixed-mode debugging for C++/C# combination
- Vulkan validation layers for graphics debugging
- ImGui integration for runtime debugging UI

## Common Issues

### Build Problems

- Run `bootstrap.bat` if vcpkg dependencies are missing
- Ensure .NET 8.0 SDK is installed and in PATH
- Check Vulkan SDK installation for graphics support

### Runtime Errors

- Verify component struct layouts match between C++ and C#
- Check .NET runtime DLL presence in executable directory
- Review log files for detailed error information

## Current Game Implementation

The current game is a 3D vampire survivor where:

- **Player**: Brown monkey controlled with WASD + spacebar for jumping
- **Enemies**: Red/purple sword models that chase the player
- **Camera**: Third-person mouse-controlled camera system
- **Physics**: Gravity for player, ground collision detection
- **Game Over**: Triggered when vampire catches player (distance < 0.5)

The game demonstrates the engine's capabilities including 3D rendering, physics, input handling, and cross-language ECS systems.
