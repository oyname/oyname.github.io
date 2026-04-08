# KROM Engine Build, CMake, and Setup Reference

## Overview

KROM Engine uses CMake as its primary build system. The engine is split into modular targets for the core runtime, platform layers, renderer backends, and rendering features. Applications and example programs are built by linking the required targets together.

This structure keeps the engine modular and makes it possible to build only the components required for a specific backend or platform.

---

## Requirements

### Required Tools

- CMake 3.22 or newer
- A C++20-capable compiler

### Supported Toolchains

#### Windows
- Visual Studio 2022 or newer
- Ninja is supported
- Win32 platform layer is available
- Direct3D 11 and OpenGL backends are supported

#### Linux
- Ninja or Make-based CMake generators
- GLFW platform layer is supported when GLFW headers and libraries are available
- OpenGL backend is supported

### External Dependencies

- Threads package via CMake:
  - `find_package(Threads REQUIRED)`
- GLFW is optional and only required when building the GLFW platform layer
- Direct3D 11 uses system-provided Windows SDK components
- OpenGL uses platform-provided OpenGL libraries

---

## Quick Start

### Configure and build with Visual Studio

```bash
cmake -S . -B build
cmake --build build --config Debug
```

### Configure and build with Ninja

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Build configurations

With multi-config generators such as Visual Studio, the active configuration is selected with:

```bash
cmake --build build --config Debug
cmake --build build --config Release
```

With single-config generators such as Ninja, the build type is selected during configuration:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

---

## Output Layout

The default output directories are:

- Executables: `build/bin`
- Static and shared libraries: `build/lib`

These are controlled by:

```cmake
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
```

---

## Core CMake Options

### `KROM_USE_GLFW`

Enables the GLFW platform layer when GLFW headers and libraries are found.

- Type: `BOOL`
- Default: `ON`

Example:

```bash
cmake -S . -B build -DKROM_USE_GLFW=OFF
```

### `KROM_BUILD_DX11`

Controls whether the Direct3D 11 addon is built.

- Type: `BOOL`
- Availability: Windows only

### `KROM_BUILD_OPENGL`

Controls whether the OpenGL addon is built.

- Type: `BOOL`

Example:

```bash
cmake -S . -B build -DKROM_BUILD_DX11=ON -DKROM_BUILD_OPENGL=ON
```

Only options that are actually defined in the active CMake configuration should be treated as supported build switches.

---

## Target Structure

### `engine_core`

The main engine library.

It contains the platform-independent core systems, including:

- ECS
- scene systems
- asset pipeline
- collision queries
- renderer abstraction
- render graph
- renderer runtime orchestration
- platform-independent utility systems

This target is the base dependency for all platform layers and renderer backends.

---

### `engine_platform_win32`

Windows-specific platform layer.

It provides:

- window creation and management on Win32
- input handling
- Win32 threading integration

This target is intended for native Windows builds.

---

### `engine_platform_glfw`

GLFW-based platform layer.

It provides:

- cross-platform window creation
- input handling through GLFW
- GLFW thread/platform integration

This target is only available when GLFW is found during configuration.

---

### `engine_dx11`

Direct3D 11 renderer backend.

This addon provides the Direct3D 11 implementation used by the renderer abstraction layer. It is only available on Windows.

---

### `engine_opengl`

OpenGL renderer backend.

This addon provides the OpenGL implementation used by the renderer abstraction layer.

---

### `engine_forward`

Forward-rendering feature addon.

This target adds the forward rendering pipeline and feature registration required by the example render loop programs.

---

### `engine_null`

Null backend or fallback addon.

This target is useful for testing, headless execution, or cases where a renderer backend is intentionally replaced with a non-rendering implementation.

---

## Self-Registering Addons

Some engine addons register themselves through static initialization. In those cases, linking the addon with `target_link_libraries(...)` alone may not be sufficient, because the linker can discard unreferenced object files from static libraries.

To force inclusion of self-registering addons, KROM Engine uses:

```cmake
krom_link_self_registering_addon(target addon_target)
```

### Example

```cmake
add_executable(my_app src/main.cpp)

target_link_libraries(my_app PRIVATE
    engine_core
    engine_forward
    engine_platform_win32
)

krom_link_self_registering_addon(my_app engine_dx11)
```

This helper applies the platform-specific linker options required to keep the addon registration code in the final binary.

---

## Example Targets

The example programs included in the engine build are backend-specific render-loop applications.

### `engine_dx11_render_loop`

Minimal render-loop example for the Direct3D 11 backend.

Dependencies:

- `engine_core`
- `engine_forward`
- `engine_platform_win32`
- `engine_dx11`

Source file:

```text
examples/dx11_render_loop.cpp
```

This target is only available when both `engine_dx11` and `engine_platform_win32` are available.

---

### `engine_opengl_render_loop`

Minimal render-loop example for the OpenGL backend.

Dependencies depend on platform:

#### On Windows
- `engine_core`
- `engine_forward`
- `engine_platform_win32`
- `engine_opengl`

#### On GLFW-based builds
- `engine_core`
- `engine_forward`
- `engine_platform_glfw`
- `engine_opengl`

Source file:

```text
examples/opengl_render_loop.cpp
```

This target is built when the OpenGL addon is available and a compatible platform layer is present.

---

## Asset Deployment for Examples

The render-loop example targets copy the `assets` directory into the executable output directory after build.

This is typically implemented with a post-build step such as:

```cmake
add_custom_command(TARGET engine_dx11_render_loop POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:engine_dx11_render_loop>/assets)
```

The same pattern is used for the OpenGL example target.

---

## Integrating the Engine Into Your Own Project

### Direct3D 11 application on Windows

```cmake
add_executable(my_game src/main.cpp)

target_link_libraries(my_game PRIVATE
    engine_core
    engine_forward
    engine_platform_win32
)

krom_link_self_registering_addon(my_game engine_dx11)
```

This produces a Windows application using the engine core, the forward feature set, the Win32 platform layer, and the Direct3D 11 backend.

---

### OpenGL application on Windows

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

### OpenGL application with GLFW

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

## Backend Selection

The current example programs are built as backend-specific executables.

- `engine_dx11_render_loop` links only the Direct3D 11 backend
- `engine_opengl_render_loop` links only the OpenGL backend

This keeps the example binaries explicit and avoids ambiguity during debugging.

If an application is intended to support runtime backend selection, that behavior must be implemented explicitly in application code and in the way renderer backends are linked.

---

## Recommended Documentation Layout

For engine documentation, the CMake and build reference should be kept separate from the gameplay or rendering API reference.

A practical structure is:

- Build system overview
- Requirements
- Quick start
- CMake options
- Target reference
- Addon registration
- Example targets
- Application integration

This keeps the build reference technical, compact, and directly usable.
