# KROM Engine: CMake, Build, and Setup

## Overview

KROM Engine uses CMake to organize and build the project.

The engine is split into separate modules instead of being built as one large block. That makes it easier to maintain different platform layers, renderer backends, and rendering features without mixing everything together.

In practice, the build setup is based on these parts:

- `engine_core` for the shared engine systems
- a platform layer such as `engine_platform_win32`
- a renderer backend such as `engine_dx11` or `engine_opengl`
- a rendering feature package such as `engine_forward`
- example applications that combine those parts

A useful way to think about it is:

```text
application
    -> renderer backend
    -> rendering feature
    -> platform layer
    -> engine core
```

---

## What you need

Before building the engine, make sure the required tools are installed.

### Required

- CMake 3.22 or newer
- a C++20-capable compiler

### Windows setup

For Windows development, the normal setup is:

- Visual Studio 2022 or newer
- Desktop development with C++
- Windows SDK
- CMake

This is enough for the Win32 platform layer and the Direct3D 11 backend.

### Optional

- Ninja, if you prefer Ninja instead of Visual Studio as the CMake generator
- GLFW, if you want to use the GLFW platform layer and the required headers and libraries are available on your system

---

## Project structure in build terms

The build system defines several targets. The most important ones are:

### `engine_core`

This is the main engine library.

It contains the shared engine code, including:

- ECS
- scene systems
- asset pipeline
- collision queries
- renderer abstraction
- render graph
- renderer runtime systems

This is the base module that the other engine targets depend on.

---

### `engine_platform_win32`

This is the Windows platform layer.

It provides the Win32-specific code for:

- window creation
- input handling
- threading integration

This target is used for native Windows builds.

---

### `engine_platform_glfw`

This is the GLFW-based platform layer.

It provides:

- window creation through GLFW
- input handling through GLFW
- platform integration through GLFW

This target is only available when GLFW is found during CMake configuration.

---

### `engine_dx11`

This is the Direct3D 11 renderer backend.

It contains the DX11-specific renderer implementation and is only available on Windows.

---

### `engine_opengl`

This is the OpenGL renderer backend.

It contains the OpenGL-specific renderer implementation.

---

### `engine_forward`

This target contains the forward-rendering feature set used by the render-loop examples.

---

### `engine_null`

This is a null or fallback addon. It is useful for tests, headless scenarios, or cases where a non-rendering backend is needed.

---

## The example programs

The project builds a set of small, focused example applications, generated **once per
enabled renderer backend** (DX11, Vulkan, OpenGL). The per-backend targets are created
by helper functions in the root `CMakeLists.txt`
(`krom_add_wrapper_example`, `krom_add_wrapper_example_src`, `krom_add_editor`).

### Example source files

Each example is a single source file under `examples/`:

```text
examples/adapter_enum.cpp
examples/minimal_window.cpp
examples/textured_quad.cpp
examples/textured_cube.cpp
examples/ibl_sphere.cpp
examples/load_3d_model.cpp
examples/editor_scene_runtime.cpp
```

### Generated targets

For each enabled backend `<bk>` (`dx11`, `vulkan`, `opengl`), the build produces:

```text
adapter_enum_<bk>
minimal_window_<bk>
textured_quad_<bk>
textured_cube_<bk>
ibl_sphere_<bk>
load_3d_model_<bk>
editor_scene_runtime_<bk>
krom_editor_<bk>          # only when KROM_BUILD_EDITOR is ON
```

For example, the DX11 build yields `minimal_window_dx11`, `textured_cube_dx11`,
`ibl_sphere_dx11`, `krom_editor_dx11`, and so on.

### Common dependencies

The examples are built on top of the engine's *wrapper* layer
(`engine_krom_wrapper_win32` / `engine_krom_wrapper_glfw`), which aggregates the
renderer orchestration layer and the feature AddOns (forward, pbr, shadow, lighting,
mesh_renderer, camera, debug_draw, …). The platform layer and the backend AddOn are
added per target, with the backend linked through
`krom_link_self_registering_addon(<target> engine_<bk>)`.

---

## Basic build flow

The build process happens in two steps.

### 1. Configure

This tells CMake to read `CMakeLists.txt`, resolve options, detect available dependencies, and generate build files.

```bash
cmake -S . -B build
```

### 2. Build

This tells the selected build tool to compile the project.

For Visual Studio:

```bash
cmake --build build --config Debug
```

For Release:

```bash
cmake --build build --config Release
```

---

## What `-S` and `-B` mean

These two options confuse many people at the beginning.

```bash
cmake -S . -B build
```

means:

- `-S .` → use the current folder as the source directory
- `-B build` → place generated build files into the `build` folder

So CMake reads the project from the current folder and writes all generated build output into `build`.

This is the normal and clean way to work with CMake.

---

## Visual Studio vs Ninja

KROM Engine can be built with different generators.

### Visual Studio

Visual Studio is a multi-configuration generator. That means Debug and Release can exist in the same build tree.

Typical commands:

```bash
cmake -S . -B build
cmake --build build --config Debug
cmake --build build --config Release
```

### Ninja

Ninja is usually used as a single-configuration generator. That means the build type is chosen during configuration.

Typical commands:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

If you use Ninja and want a Release build, configure a Release build tree explicitly.

---

## Output directories

The engine uses fixed output folders for build artifacts.

- executables go to `build/bin`
- libraries go to `build/lib`

This is controlled by these CMake settings:

```cmake
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
```

This makes it easier to find the built examples and libraries after compilation.

---

## Main CMake options

### `KROM_USE_GLFW`

This option enables the GLFW platform layer when GLFW can be found.

- Type: `BOOL`
- Default: `ON`

Example:

```bash
cmake -S . -B build -DKROM_USE_GLFW=OFF
```

---

### `KROM_BUILD_DX11`

This option controls whether the Direct3D 11 addon is built.

It is mainly relevant on Windows.

Example:

```bash
cmake -S . -B build -DKROM_BUILD_DX11=ON
```

---

### `KROM_BUILD_OPENGL`

This option controls whether the OpenGL addon is built.

Example:

```bash
cmake -S . -B build -DKROM_BUILD_OPENGL=ON
```

---

### `KROM_BUILD_VULKAN`

Controls whether the Vulkan backend addon is built.

- Type: `BOOL`
- Default: `ON`

---

### `KROM_BUILD_EDITOR`

Controls whether the in-app ImGui editor (`krom_editor_<backend>` targets) is built.

- Type: `BOOL`
- Default: `ON`

---

### Example: build both DX11 and OpenGL

```bash
cmake -S . -B build -DKROM_BUILD_DX11=ON -DKROM_BUILD_OPENGL=ON
cmake --build build --config Debug
```

---

## What `target_link_libraries(...)` means in this project

In CMake, targets are connected to the libraries they use.

Example:

```cmake
target_link_libraries(krom_pbr_shadow_dx11 PRIVATE
    engine_core
    engine_renderer_orchestration_layer
    engine_forward
    engine_platform_win32
)
```

This means that `krom_pbr_shadow_dx11` is built with access to those engine modules.

In other words, the example program can use the code contained in those libraries.

For this project, that is the most useful way to read it. You do not need deeper linker details at the beginning.

---

## Self-registering addons

Some addons register themselves automatically through static initialization.

Because of that, a normal link step is not always enough. A linker may remove code it considers unused, which can break automatic registration.

To avoid that, the engine uses this helper:

```cmake
krom_link_self_registering_addon(target addon_target)
```

Example:

```cmake
add_executable(my_app src/main.cpp)

target_link_libraries(my_app PRIVATE
    engine_core
    engine_forward
    engine_platform_win32
)

krom_link_self_registering_addon(my_app engine_dx11)
```

In simple terms, this makes sure the backend addon is really included in the final executable.

---

## Adding your own application

If you want to build your own application on top of the engine, the pattern is the same as in the examples.

### Windows + Direct3D 11

```cmake
add_executable(my_game src/main.cpp)

target_link_libraries(my_game PRIVATE
    engine_core
    engine_forward
    engine_platform_win32
)

krom_link_self_registering_addon(my_game engine_dx11)
```

This gives your application:

- the engine core
- the Win32 platform layer
- the forward rendering feature
- the DX11 backend

---

### Windows + OpenGL

```cmake
add_executable(my_game src/main.cpp)

target_link_libraries(my_game PRIVATE
    engine_core
    engine_forward
    engine_platform_win32
)

krom_link_self_registering_addon(my_game engine_opengl)
```

---

### GLFW + OpenGL

```cmake
add_executable(my_game src/main.cpp)

target_link_libraries(my_game PRIVATE
    engine_core
    engine_forward
    engine_platform_glfw
)

krom_link_self_registering_addon(my_game engine_opengl)
```

---

## Asset copy step in the examples

The example targets copy the `assets` directory into the executable output folder after the build.

That is why the examples can usually run directly after compilation without manually moving assets.

The CMake pattern looks like this:

```cmake
add_custom_command(TARGET krom_pbr_shadow_dx11 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:krom_pbr_shadow_dx11>/assets)
```

The Vulkan and OpenGL examples use the same pattern.

---

## How to read the main `CMakeLists.txt`

Do not try to understand the whole file from top to bottom in one pass.

A better reading order is:

1. project setup
2. `engine_core`
3. platform layers
4. addons
5. example targets

That order matches the actual dependency structure and makes the file much easier to follow.

---

## Minimal mental model

If you only want the practical picture, this is enough:

### DX11 example (e.g. `ibl_sphere_dx11`)

```text
ibl_sphere_dx11
    -> engine_dx11   (via krom_link_self_registering_addon)
    -> engine_krom_wrapper_win32  (forward + pbr + shadow + lighting + ...)
    -> engine_platform_win32
    -> engine_core
```

### Vulkan example (e.g. `ibl_sphere_vulkan`)

```text
ibl_sphere_vulkan
    -> engine_vulkan (via krom_link_self_registering_addon)
    -> engine_krom_wrapper_win32  (forward + pbr + shadow + lighting + ...)
    -> engine_platform_win32
    -> engine_core
```

### OpenGL example (e.g. `ibl_sphere_opengl`)

```text
ibl_sphere_opengl
    -> engine_opengl (via krom_link_self_registering_addon)
    -> engine_krom_wrapper_win32 or engine_krom_wrapper_glfw
    -> engine_platform_win32 or engine_platform_glfw
    -> engine_core
```

That model is accurate enough for everyday work and simple enough to keep in mind.

---

## Recommended workflow

A practical workflow for working on the engine is:

1. configure the project
2. build Debug
3. run one example
4. only then change CMake structure if needed
5. after CMake changes, reconfigure
6. if CMake starts behaving inconsistently, clear the build directory and configure again

That avoids a lot of unnecessary confusion while changing targets or file lists.

---

## Summary

The important points are simple:

- CMake organizes and generates the build
- the engine is split into modules instead of one large target
- `engine_core` is the base of the engine
- platform layers handle OS-specific integration
- DX11, Vulkan, and OpenGL are separate renderer backend addons
- example programs are generated per backend (e.g. `minimal_window_dx11`, `ibl_sphere_vulkan`, `textured_cube_opengl`, `krom_editor_dx11`) from single source files under `examples/`
- self-registering addons must be linked with the helper function, not only with a normal library link

That is the build setup in practical terms.

