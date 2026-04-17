# KROM Engine AddOn API Reference

## Purpose

This document defines the AddOn model for KROM Engine.
It is intentionally general and should be used as the baseline reference for anyone who wants to create new engine extensions.

An AddOn is a self-contained module that extends the engine **without modifying the core contract**.
AddOns are the approved extension mechanism for:

- rendering backends
- render features and pipelines
- material/model helpers
- backend-specific shader reflection
- optional runtime integrations

The core engine remains API-neutral and opinion-free. Concrete implementations belong in `addons/<name>/`.

---

## Design Goals

The AddOn system exists to enforce these rules:

- **Core owns abstractions.**
- **AddOns own concrete implementations.**
- **Build dependencies are explicit.**
- **Registration is explicit or self-registering by design.**
- **No hidden coupling between core and individual AddOns.**

If a module has no isolated build target, no clear public API, and no defined registration path, then it is not a real AddOn.

---

## AddOn Categories

KROM currently supports two main AddOn categories.

### 1. Backend AddOns

Backend AddOns provide concrete `IDevice` implementations and related backend-specific logic.

Typical examples:

- `dx11`
- `opengl`
- `vulkan`
- `null`

Typical contents:

- device implementation
- swapchain implementation
- command list implementation
- resource implementation
- shader reflector
- backend-specific enumeration
- backend registration

### 2. Feature AddOns

Feature AddOns extend the rendering framework through `IEngineFeature`.

Typical examples:

- `forward`
- future `deferred`
- future `toon`
- future `postfx`

Typical contents:

- scene extraction steps
- render pipeline definitions
- feature initialization and shutdown
- feature-specific runtime helpers

### 3. Utility / Material AddOns

These AddOns provide convenience APIs or content-side helpers without changing core abstractions.

Typical examples:

- `pbr`
- future material helper libraries
- shader authoring helpers

These usually expose helper types, builders, descriptors, or content conventions.

---

## Required AddOn Structure

Each AddOn must live in its own directory under `addons/`.

Example:

```text
addons/myaddon/
    CMakeLists.txt
    MyAddon.hpp
    MyAddon.cpp
```

Recommended structure for larger AddOns:

```text
addons/myaddon/
    CMakeLists.txt
    include/
    src/
```

For small AddOns, flat layout is acceptable if it stays readable.

Minimum requirements:

- one dedicated folder per AddOn
- one dedicated CMake target per AddOn
- one clear public entry point
- all AddOn-local implementation details kept inside the AddOn

---

## Public API Rules

An AddOn must expose a narrow, stable surface.

### Good public API

- one or a few focused headers
- factory functions
- helper types that map to engine concepts
- no leaking of unrelated internal implementation types

### Bad public API

- random internal headers included directly from app code
- exposing backend internals through the core
- forcing users to include private implementation files
- mixing registration, rendering, and asset logic in one giant header

Recommended pattern:

```cpp
#pragma once

namespace engine::renderer::addons::myfeature {

// Public entry point(s) only.

} // namespace engine::renderer::addons::myfeature
```

---

## Build Integration

Every AddOn must define its own static library target.

Minimal example:

```cmake
set(MYADDON_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/MyAddon.cpp
)

add_library(engine_myaddon STATIC ${MYADDON_SOURCES})

target_include_directories(engine_myaddon
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_link_libraries(engine_myaddon PUBLIC engine_core)
```

### Rules

- AddOn targets must link against `engine_core` if they use core engine APIs.
- AddOn-specific include directories must be exported through the target.
- Backend-specific system dependencies must stay in the AddOn target, not in `engine_core`.
- The root `CMakeLists.txt` may include the AddOn through `add_subdirectory(addons/<name>)`.

Example root integration:

```cmake
add_subdirectory(addons/myaddon)
```

### Important

If `add_subdirectory(addons/<name>)` is used, that folder must contain a valid `CMakeLists.txt`.
Without it, configuration fails immediately.

---

## Registration Models

KROM uses two different registration models depending on AddOn type.

### 1. Backend self-registration

Backend AddOns register themselves through `DeviceFactory::Registrar`.

Relevant engine contract:

- `engine::renderer::DeviceFactory`
- `engine::renderer::IDevice`

Backend registration pattern:

```cpp
#include "renderer/IDevice.hpp"

namespace engine::renderer::addons::mybackend {
namespace {

std::unique_ptr<IDevice> CreateMyBackendDevice()
{
    return std::make_unique<MyBackendDevice>();
}

std::vector<AdapterInfo> EnumerateMyBackendAdapters()
{
    return {};
}

DeviceFactory::Registrar g_myBackendRegistrar(
    DeviceFactory::BackendType::Vulkan,
    &CreateMyBackendDevice,
    &EnumerateMyBackendAdapters,
    false
);

} // namespace
} // namespace engine::renderer::addons::mybackend
```

### Backend registration rules

- registration must happen inside the AddOn
- the core must not manually know backend implementation classes
- the chosen `BackendType` must match the actual backend
- enumeration is optional but strongly recommended for real hardware backends
- stub backends must declare themselves as such when appropriate

### 2. Feature registration

Feature AddOns expose `IEngineFeature` implementations.
They are then registered into the render system.

Relevant engine contract:

- `engine::renderer::IEngineFeature`
- `engine::renderer::FeatureRegistry`
- `engine::renderer::RenderSystem::RegisterFeature(...)`

Factory pattern:

```cpp
#pragma once

#include "renderer/FeatureRegistry.hpp"
#include <memory>

namespace engine::renderer::addons::myfeature {

std::unique_ptr<IEngineFeature> CreateMyFeature();

} // namespace engine::renderer::addons::myfeature
```

Implementation pattern:

```cpp
#include "MyFeature.hpp"

namespace engine::renderer::addons::myfeature {
namespace {

class MyFeature final : public IEngineFeature
{
public:
    std::string_view GetName() const noexcept override { return "krom-myfeature"; }
    FeatureID GetID() const noexcept override { return FeatureID::FromString("krom-myfeature"); }

    void Register(FeatureRegistrationContext& context) override
    {
        // Register extraction steps and/or pipelines here.
    }

    bool Initialize(const FeatureInitializationContext& context) override
    {
        (void)context;
        return true;
    }

    void Shutdown(const FeatureShutdownContext& context) override
    {
        (void)context;
    }
};

} // namespace

std::unique_ptr<IEngineFeature> CreateMyFeature()
{
    return std::make_unique<MyFeature>();
}

} // namespace engine::renderer::addons::myfeature
```

Application or bootstrap registration:

```cpp
renderSystem.RegisterFeature(engine::renderer::addons::myfeature::CreateMyFeature());
```

### Feature registration rules

- feature IDs must be stable and unique
- dependencies must be declared through `GetDependencies()` when needed
- `Register(...)` must only register feature-owned pipeline and extraction objects
- `Initialize(...)` must not assume unrelated AddOns are present unless declared as dependencies
- `Shutdown(...)` must leave no persistent registration or dangling ownership behind

---

## Scene Extraction and Pipeline Extension

Feature AddOns are the approved place for render pipeline composition.

They may contribute:

- scene extraction steps via `ISceneExtractionStep`
- render pipelines via `IRenderPipeline`

### Scene extraction step contract

Use extraction steps to move scene/ECS data into `RenderWorld`.

Example:

```cpp
class MyExtractionStep final : public ISceneExtractionStep
{
public:
    std::string_view GetName() const noexcept override { return "myfeature.extract"; }

    void Extract(const ecs::World& world, RenderWorld& renderWorld) const override
    {
        (void)world;
        (void)renderWorld;
    }
};
```

### Render pipeline contract

Use `IRenderPipeline` to build frame graph content.

Example:

```cpp
class MyPipeline final : public IRenderPipeline
{
public:
    std::string_view GetName() const noexcept override { return "myfeature"; }

    bool Build(const RenderPipelineBuildContext& context,
               RenderPipelineBuildResult& result) const override
    {
        (void)context;
        result.resources = {};
        return true;
    }
};
```

### Rules

- extraction steps should be deterministic and side-effect free beyond render extraction
- pipelines should consume engine abstractions, not backend internals
- feature-owned registrations must stay owned by the feature that created them

---

## Material and Helper AddOns

Not every AddOn needs to register a backend or a feature.
Some AddOns simply provide optional engine-facing helper APIs.

Typical examples:

- material descriptors
- helper builders
- shader parameter packing helpers
- engine-specific authoring convenience wrappers

Example public API shape:

```cpp
#pragma once

namespace engine::renderer::addons::pbr {

struct PbrMaterialDesc
{
    // helper-facing data only
};

class PbrMaterial
{
public:
    // helper API
};

} // namespace engine::renderer::addons::pbr
```

### Rules

- helper AddOns must build as separate targets
- helper AddOns must not inject hardcoded policy into the core
- helper AddOns must not silently replace core abstractions
- app targets that use helper headers must link the helper AddOn target and include its public path

---

## Linking Rules for Applications

An application must explicitly link the AddOns it uses.

Example:

```cmake
target_link_libraries(my_app PRIVATE
    engine_core
    engine_forward
    engine_pbr
    engine_platform_win32
)
```

If the application directly includes AddOn headers, the AddOn include path must be visible through the target or added explicitly.

Example:

```cmake
target_include_directories(my_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/addons/forward
    ${CMAKE_CURRENT_SOURCE_DIR}/addons/pbr
)
```

### Important

Using an AddOn header without linking the matching AddOn target is not a valid integration model.
Build configuration and source usage must match.

---

## Self-Registering Static Libraries

Some AddOns rely on static initialization for registration.
This is especially relevant for backend AddOns using `DeviceFactory::Registrar`.

When linking static libraries, the linker may discard object files whose symbols are not referenced directly.
That can break self-registration.

KROM therefore provides a helper function in the root CMake configuration:

```cmake
function(krom_link_self_registering_addon target addon_target)
    target_link_libraries(${target} PRIVATE ${addon_target})
    if(MSVC)
        target_link_options(${target} PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${addon_target}>")
    elseif(APPLE)
        target_link_options(${target} PRIVATE "LINKER:-force_load,$<TARGET_FILE:${addon_target}>")
    else()
        target_link_options(${target} PRIVATE
            "LINKER:--whole-archive,$<TARGET_FILE:${addon_target}>,--no-whole-archive")
    endif()
endfunction()
```

Use it for self-registering AddOns:

```cmake
krom_link_self_registering_addon(my_app engine_dx11)
```

### Rule

If an AddOn depends on static initialization for registration, it must be linked in a way that guarantees retention of the registration object.

---

## Include and Dependency Policy

### Core may depend on

- public engine headers
- standard library
- shared abstractions and registries

### Core must not depend on

- backend AddOn headers
- PBR helper headers
- forward pipeline implementation headers
- any concrete AddOn-private type

### AddOns may depend on

- `engine_core`
- platform targets when justified
- system libraries required by the AddOn
- other AddOns only when the dependency is explicit and stable

### AddOns should avoid

- circular AddOn dependencies
- reaching into unrelated AddOn internals
- leaking backend types into general feature APIs

---

## Naming Conventions

Recommended naming:

- folder: `addons/<name>`
- target: `engine_<name>`
- namespace: `engine::renderer::addons::<name>`
- feature ID string: `krom-<name>`

Examples:

- `addons/forward` -> `engine_forward`
- `addons/pbr` -> `engine_pbr`
- `addons/vulkan` -> `engine_vulkan`

This keeps discovery, build naming, and code naming aligned.

---

## Minimal Backend AddOn Example

### File layout

```text
addons/sample_backend/
    CMakeLists.txt
    SampleBackendDevice.hpp
    SampleBackendDevice.cpp
```

### CMake

```cmake
set(SAMPLE_BACKEND_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleBackendDevice.cpp
)

add_library(engine_sample_backend STATIC ${SAMPLE_BACKEND_SOURCES})

target_include_directories(engine_sample_backend
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_link_libraries(engine_sample_backend PUBLIC engine_core)
```

### Registration

```cpp
#include "renderer/IDevice.hpp"
#include "SampleBackendDevice.hpp"

namespace engine::renderer::addons::sample_backend {
namespace {

std::unique_ptr<IDevice> CreateDevice()
{
    return std::make_unique<SampleBackendDevice>();
}

DeviceFactory::Registrar g_reg(
    DeviceFactory::BackendType::Null,
    &CreateDevice,
    nullptr,
    false
);

} // namespace
} // namespace engine::renderer::addons::sample_backend
```

---

## Minimal Feature AddOn Example

### File layout

```text
addons/sample_feature/
    CMakeLists.txt
    SampleFeature.hpp
    SampleFeature.cpp
```

### Public header

```cpp
#pragma once

#include "renderer/FeatureRegistry.hpp"
#include <memory>

namespace engine::renderer::addons::sample_feature {

std::unique_ptr<IEngineFeature> CreateSampleFeature();

} // namespace engine::renderer::addons::sample_feature
```

### Implementation

```cpp
#include "SampleFeature.hpp"

namespace engine::renderer::addons::sample_feature {
namespace {

class SampleFeature final : public IEngineFeature
{
public:
    std::string_view GetName() const noexcept override { return "krom-sample-feature"; }
    FeatureID GetID() const noexcept override { return FeatureID::FromString("krom-sample-feature"); }

    void Register(FeatureRegistrationContext& context) override
    {
        (void)context;
    }

    bool Initialize(const FeatureInitializationContext& context) override
    {
        (void)context;
        return true;
    }

    void Shutdown(const FeatureShutdownContext& context) override
    {
        (void)context;
    }
};

} // namespace

std::unique_ptr<IEngineFeature> CreateSampleFeature()
{
    return std::make_unique<SampleFeature>();
}

} // namespace engine::renderer::addons::sample_feature
```

### App integration

```cpp
renderSystem.RegisterFeature(engine::renderer::addons::sample_feature::CreateSampleFeature());
```

---

## What an AddOn Must Not Do

An AddOn must not:

- modify core invariants from the outside
- require the core to include AddOn-private headers
- hide registration in unrelated engine code
- create silent side dependencies on unrelated AddOns
- hardcode assumptions that belong in content or app bootstrap
- bypass engine abstraction layers without a clearly documented reason

---

## Validation Checklist

Before calling a module an AddOn, verify all of the following:

- it lives in `addons/<name>/`
- it has its own `CMakeLists.txt`
- it builds as a dedicated target
- it exposes a clear public API
- it has a documented registration path
- it does not require core-private hacks
- app targets explicitly link it when they use it
- its ownership and lifecycle are clear

If any of these are missing, the module is not finished.

---

## Recommended Extension Workflow

When creating a new AddOn, follow this order:

1. define the AddOn category
2. define the public API surface
3. create the AddOn build target
4. implement registration
5. integrate it into the root build
6. link it explicitly from example or app targets
7. verify that no core code now depends on AddOn-private details

---

## Final Rule

KROM AddOns are not cosmetic folders.
They are architectural boundaries.

A proper AddOn is:

- isolated
- buildable
- explicit
- registerable
- replaceable

If those properties are missing, the design has not been finished.
