# Design Decisions & Development Plan

This document outlines the current design of our C# game plugin system, recent utility additions, and a development plan for future enhancements.

## 1. Recent Utility Script (Python Logger)

To aid in development, inspection, and sharing of the C# project's state, a Python utility script (`log_game_files.py`) has been introduced.

**Purpose:**

- Log the full directory structure of the `./game/` C# project.
- Log the relative path and content of each file within the `./game/` project.

**Key Features:**

- Excludes `./game/bin/` and `./game/obj/` directories from logging.
- Copies the entire logged output to the system clipboard for easy sharing.
- Notifies the user in the command line about the clipboard operation status.
- Minimal external dependencies (relies on standard Python libraries and common OS clipboard utilities).

**Benefit:**
This script provides a quick and consistent way to capture the current state of the C# source files, making it easier to discuss changes, debug issues, and share project snapshots without manually copying files or structures.

## 2. Core C#/.NET Host Design Overview

Our current system allows C# code to define game logic, which is then hosted and executed by a C++ application.

- **C# Game Logic (`Game.dll`):**
  - Game systems and components are written in C#.
  - Attributes like `[StartupSystem]`, `[UpdateSystem]`, and `[Query(typeof(T))]` are used to identify and configure systems.
  - `GameScript.cs` contains example systems.
  - `Components.cs` defines shared data structures (e.g., `Transform`, `Velocity`) with `[StructLayout(LayoutKind.Sequential)]` for C++ interop.
- **C++ Host Application:**
  - Loads the .NET Core runtime (via `hostfxr`).
  - Loads the `Game.dll` assembly.
- **Plugin Bootstrap (`PluginBootstrap.cs`):**
  - The `RegisterAll` method (marked with `[UnmanagedCallersOnly]`) is the entry point called by the C++ host.
  - It uses .NET Reflection to scan the assembly for methods marked with system attributes.
  - For each system, it dynamically generates a "shim" method using `System.Reflection.Emit` (IL Emit). This shim adapts the C# system's signature to a `NativePerEntityDel` or `ManagedStartupDel` delegate signature suitable for P/Invoke from C++.
  - These shims are then registered with the C++ host.
- **Communication & Interop:**
  - **C# to C++:** `EngineBindings.cs` defines C# delegates (e.g., `CreateEntityDel`) that are bound to function pointers provided by the C++ host. This allows C# to call functions implemented in C++ (e.g., `EngineBindings.CreateEntity()`).
  - **C++ to C#:** The C++ host calls the `RegisterAll` function pointer. It also receives function pointers to the generated shims from C# and calls them during the game loop (for update systems) or at startup.
  - The C++ host provides a `HostGetProcAddress` function, which C# uses to resolve the addresses of C++-side functions.
  - Data (like component pointers) is passed as raw pointers (`void*`) between C++ and the C# shims. The shims then cast/convert these to the appropriate C# types (e.g., `Transform*`, which is then passed as `ref Transform` to the actual system method).
- **Current Limitations:**
  - `[Query]` attribute currently only supports querying for entities with a single component type per system.
  - Component definitions are duplicated (structs in C# and C++).
  - No explicit mechanism for component or entity deletion from the C# side is defined in `EngineBindings`.

## 3. Development Plan

The following enhancements are planned to increase the flexibility and power of our game scripting system.

### 3.1. Support Multi-Component Queries

- **Current State:** Systems like `UpdateTransform(float dt, ref Transform t)` can only operate on entities based on a single component type specified in `[Query(typeof(Transform))]`.
- **Goal:** Allow systems to query for and operate on entities that possess multiple specific components (e.g., an entity that has _both_ a `Transform` AND a `Velocity` component).
  - Example C# system signature: `public static void PhysicsSystem(float dt, ref Transform t, ref Velocity v)`
  - Attribute usage: `[UpdateSystem, Query(typeof(Transform), typeof(Velocity))]`
- **Proposed Changes:**
  1.  **C# `PluginBootstrap.cs`:**
      - The `QueryAttribute` already accepts `params Type[]`, so its definition is fine.
      - The IL emission logic within `RegisterAll` for `[UpdateSystem]` methods needs significant changes:
        - The `shim` delegate `NativePerEntityDel` currently takes `void* componentsPtr`. This `componentsPtr` (which is `ptrs.data()` from C++) will point to an array of component pointers (`T1*`, `T2*`, ...).
        - The IL code must load each component pointer from this array and pass them as separate arguments to the target C# system method.
          - For `comps[0]`: `Ldarg_1` (loads `componentsPtr`), `Ldc_I4_0` (index 0), `Conv_I`, `Add`, `Ldind_I` (gets `T1*`).
          - For `comps[1]`: `Ldarg_1` (loads `componentsPtr`), `Ldc_I4_S <size_of_pointer>`, `Conv_I`, `Add` (advances to `&ptrs[1]`), `Ldind_I` (gets `T2*`).
          - This needs to be repeated for all components in the query.
        - The `OpCodes.Call` to the user's system method will now have multiple component arguments.
  2.  **C++ Host (`HostRegisterPerEntityUpdate`):**
      - The `names` array already provides the component type names.
      - The lambda function registered as an update system needs to:
        - Construct an `entt::runtime_view` that includes _all_ component types specified in the `names` array.
        - When iterating entities in the view:
          - The `std::vector<void*> ptrs` must be populated with pointers to _each_ of the queried components for the current entity, in the order specified by `names`.
          - Example: `ptrs[0] = &registry.get<Transform>(e); ptrs[1] = &registry.get<Velocity>(e);`
- **Challenges:**
  - Ensuring the correct order and type of arguments when emitting IL for the `call` instruction.
  - Matching the order of pointers populated in the C++ `ptrs` vector with the order expected by the C# shim and the target system method.

### 3.2. Support Component/Entity Deletion

- **Current State:** Entities and components can be created and added, but there's no exposed mechanism for C# scripts to remove them.
- **Goal:** Allow C# systems to request the deletion of specific components from an entity or the deletion of an entire entity.
- **Proposed Changes:**
  1.  **C++ Host:**
      - Implement new functions that use `entt::registry` methods:
        - `void HostRemoveComponentTransform(uint32_t entityId) { AppSingleton().registry.remove<Transform>(entt::entity{entityId}); }` (repeat for `Velocity` and other components).
        - `void HostDestroyEntity(uint32_t entityId) { AppSingleton().registry.destroy(entt::entity{entityId}); }`
      - Expose these functions through `HostGetProcAddress`.
  2.  **C# `EngineBindings.cs`:**
      - Define new delegates: `delegate void RemoveComponentDel(uint entityId);`, `delegate void DestroyEntityDel(uint entityId);`.
      - Add static methods: `public static RemoveComponentDel RemoveTransform = null!;`, `public static DestroyEntityDel DestroyEntity = null!;` etc.
      - Initialize these delegates in `EngineBindings.Init()` by calling `GetProc("HostRemoveComponentTransform")`, etc.
- **Considerations:**
  - `entt` handles entities disappearing from views gracefully. If a system queries for components and an entity loses one of them (or is destroyed), it will simply no longer be part of that system's iteration.

### 3.3. Add Other Component Types

- **Goal:** Introduce new component types to expand game logic capabilities.
  - **`Mesh` Component:** To specify a 3D model.
    - Content: A string path to the model file.
  - **`Material` Component:** To specify basic appearance.
    - Content: A `System.Numerics.Vector3` representing a color.
- **Proposed Changes:**

  1.  **C# `Components.cs`:**

      ```csharp
      [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)] // For modelPath
      public struct Mesh
      {
          [MarshalAs(UnmanagedType.LPStr)] // Marshal as C-style string
          public string modelPath;
      }

      [StructLayout(LayoutKind.Sequential)]
      public struct Material
      {
          public System.Numerics.Vector3 color;
      }

      ```

  2.  **C++ (e.g., `Components.hpp`):**
      - Define corresponding C++ structs:
        - `struct Mesh { const char* modelPath; };`
        - `struct Material { Vector3 color; };` (assuming `Vector3` is defined or aliased)
  3.  **C++ Host:**
      - Add `Mesh` and `Material` to `g_getters` and `g_storage_iterators` maps.
      - Implement `AddMesh(uint32_t e, Mesh m)` and `AddMaterial(uint32_t e, Material mat)` functions (similar to `AddTransform`).
      - Expose these new `Add` functions via `HostGetProcAddress`.
  4.  **C# `EngineBindings.cs`:**
      - Add delegates `AddMeshDel(uint e, Mesh m)` and `AddMaterialDel(uint e, Material mat)`.
      - Add corresponding static methods and initialize them in `Init()`.

- **Challenges:**
  - String marshalling for `Mesh.modelPath`: `CharSet.Ansi` and `[MarshalAs(UnmanagedType.LPStr)]` are crucial for passing strings as null-terminated C-style strings. The C++ side must handle the lifetime of this string if it copies it. For simplicity, the C++ side might expect the string to be valid for the duration of the `AddMesh` call or make its own copy.

### 3.4. Optimize Component Definition (Optional Goal)

- **Current State:** Component structs must be manually defined and kept synchronized in both C# (`Components.cs`) and C++ (e.g., `Components.hpp`), including `[StructLayout(LayoutKind.Sequential)]` and field order. This is error-prone.
- **Goal:** Reduce or eliminate this duplication. Define components in a single source of truth.
- **Possible Approaches (High-level):**
  1.  **C# Source Generators:** A C# source generator could analyze component struct definitions in C# during compilation and automatically generate the corresponding C++ header file (`.hpp`). This is a powerful but complex approach requiring good knowledge of Roslyn (the .NET Compiler Platform).
  2.  **External Scripting/Build Step:**
      - A script (Python, PowerShell, etc.) could parse the C# `Components.cs` file (using regex for simple cases, or Roslyn for robustness) and generate the C++ header.
      - Alternatively, parse a simplified C++ header and generate C# structs.
  3.  **Shared Definition Language (IDL):**
      - Define components in a neutral Interface Definition Language (e.g., a custom JSON schema, XML, or a dedicated IDL like Protocol Buffers - though that might be overkill).
      - Use tools or scripts to generate both the C# and C++ struct definitions from this shared definition.
- **Challenges:**
  - Significant implementation effort for any of these solutions.
  - Ensuring accurate translation of types and memory layout (`StructLayout`).
  - Integrating the generation step into the build process smoothly.
- **Recommendation:** This is a lower priority due to its complexity but offers long-term maintainability benefits.

## 4. Task Breakdown & Suggested Order

1.  **Multi-Component Queries:** This is a foundational improvement that will enable more complex and realistic game systems.
2.  **New Component Types (Mesh, Material):** Can be developed in parallel or after multi-component queries are stable. This will allow for more visually interesting demos.
3.  **Component/Entity Deletion:** Important for dynamic game worlds and resource management.
4.  **Optimize Component Definition:** Address this last due to its complexity. It's a "quality of life" improvement for developers.
