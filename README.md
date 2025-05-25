# _VulkanECS-Engine_

A hybrid C++ + C# game engine framework with a data-oriented ECS core, GPU-instanced Vulkan rendering, and embedded .NET runtime for gameplay scripting. Designed to render thousands of 3D objects with minimal draw calls and provide a productive C# authoring experience.

# Project Spec

This project aims to develop a game development framework centered around high-performance rendering using the Vulkan API. The goal is to provide a comprehensive and accessible foundation for developers, enabling the rendering of thousands of 3D objects efficiently and accelerating the creation of high-quality 3D games and applications.

**Key Features & Objectives:**

1.  **High-Performance Vulkan Rendering:** Utilizes the Vulkan API as the backend to achieve efficient rendering, specifically targeting scenarios with a large number of on-screen 3D objects.
2.  **Advanced Rendering Techniques:** Implements state-of-the-art rendering methods based on recent computer graphics research.
3.  **Modern Architecture:** Employs an Entity Component System (ECS) architecture, inspired by data-oriented design principles for better performance and modularity.
4.  **Scripting Integration:** Provides a C# scripting interface allowing users to implement custom game logic, initialize assets, manage game loops, and handle events.
5.  **User-Centric & Flexible Framework:** Designed as a flexible base upon which developers can build their own game logic, supporting diverse and interactive experiences.
6.  **Asset Management:** Features an asset pipeline for loading, processing, and managing game resources.
7.  **Working Example:** The project includes the development of a simple interactive game to demonstrate the framework's capabilities.

## Features

- **Vulkan GPU Instancing**  
  Single draw-call per mesh+material, indirect draws, compute-shader culling.
- **Data-Oriented ECS**  
  C++ archetype storage, SoA layout, ultrafast queries, minimal GC pressure.
- **Embedded .NET (CoreCLR)**  
  High-quality JIT, AOT/ReadyToRun support, server GC, AssemblyLoadContext hot-reload.
- **C# Scripting API**  
  `Engine.CreateEntity()`, `world.AddComponent<T>()`, `[System]` classes with `OnUpdate(Span<T>...)`.
- **Asset Pipeline**  
  Load/serialize meshes, textures, shaders; auto-watch & reload.
- **Sample Game**  
  “Instanced Quads” demo showing 10 000+ moving sprites.

## Architecture Overview

1. **C++ Native Core**

   - Vulkan setup, windowing (GLFW), input, audio.
   - ECS storage & scheduler (entity creation, component SoA buffers).
   - GPUInstancingSystem: batches entities by (mesh,material), issues `vkCmdDrawIndexedIndirect`.

2. **.NET Runtime Host**

   - Embeds CoreCLR via Hosting API.
   - Loads & (AOT) compiles C# assemblies.
   - Exposes a C ABI: register components, create entities, query archetypes, invoke system delegates.

3. **C# Scripting Layer**
   - Define `[Component] struct Position { ... }` & `[System] class MoveSystem { OnUpdate(...) }`.
   - `GameBootstrap.Init()` registers components & scans systems.
   - Per-frame, native core passes `Span<T>` over unmanaged memory to C# for high-speed loops.

# For developers

_Note: this setup guide is currently for Windows exclusively_

## Bootstrapping

For initializing submodules & setup vcpkg, a package manager for our project to use.

```bash
bootstrap.bat
```

## Environment Setup

### Dotnet (_newly updated_)

- Download .NET 8.0.16 with SDK 8.0.410 from [here](https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/sdk-8.0.410-windows-x64-installer)
- You can varify your installation by checking

```shell
dotnet --list-sdks
```

and you should at least find:

```shell
8.0.410 [C:\Program Files\dotnet\sdk]
```

```shell
dotnet --list-runtimes
```

and you should at least find:

```shell
Microsoft.NETCore.App 8.0.16 [C:\Program Files\dotnet\shared\Microsoft.NETCore.App]
```

### Cmake

- Download the 3.x installer from [here](https://cmake.org/download/)
- CMake version 4.x (Newest) removed compatibility support for CMake < 3.5.

### Ninja

- Download the newest pre-compiled version of ninja from [here](https://github.com/ninja-build/ninja/releases)

- create a folder named `Ninja` under `C:\Program Files\`

- copy `ninja.exe` under `C:\Program Files\Ninja`

- add `C:\Program Files\Ninja` to PATH

### Ccache

- Download the latest pre-compiled version of ccache from [here](https://github.com/ccache/ccache/releases/tag/v4.10.2)

- Copy the entire folder under `C:\Program Files\`

- add `C:\Program Files\ccache-your.version.here-windows-x86_64` to PATH

### LLVM

- _Notice: LLVM version 18 is the highest supported version for this project (from spdlog compatibility issue)_

- For the clang compiler, that is used to compile this project, download it from [here](https://github.com/llvm/llvm-project/releases/tag/llvmorg-18.1.8)

### Vulkan SDK

- This is required in order to support vulkan validation layer.
- Just download and install the latest version from [here](https://vulkan.lunarg.com/)

## Build the project

```bash
build.bat
```

## Guidelines for development using vscode

### Recommended extensions

- `command-runner`: to run customized commands from command line

  In `.vscode/settings.json`, you may want to setup some build commands as shortcuts:

  ```plaintext
    "command-runner.commands": {
    "build windows release": "build.bat",
    "build windows debug": "build.bat --debug"
  }
  ```

- `C/C++`: only for debugging, disable the IntelliSense feature

- `clangd`: C/C++ completion, navigation, and insights, faster than the `C/C++` extension, especially in larger codebases

  There's a template settings.json file under `.vscode/`, you may want to refer to that one

- `Clang-Format`: code formatting

## Learning resources

[Vulkan Tutorial](https://vulkan-tutorial.com/)

[Bevy's ECS Design](https://bevyengine.org/learn/quick-start/getting-started/ecs/)

[EnTT Crash Course](https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system/465d90e0f5961adc460cd9d1e9358370987fbcd3)
