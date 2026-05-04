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

KROM currently supports three main AddOn categories.

### 1. Backend AddOns

Backend AddOns provide concrete `IDevice` implementations and related backend-specific logic.

Typical examples: `dx11`, `opengl`, `vulkan`, `null`

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

Typical examples: `forward`, future `deferred`, future `toon`, future `postfx`

Typical contents:

- scene extraction steps
- render pipeline definitions
- feature initialization and shutdown
- feature-specific runtime helpers

### 3. Utility / Material AddOns

These AddOns provide convenience APIs or content-side helpers without changing core abstractions.

Typical examples: `pbr`, future material helper libraries, shader authoring helpers

These usually expose helper types, builders, descriptors, or content conventions.

---

## Required AddOn Structure

Each AddOn must live in its own directory under `addons/`.

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

---

## Build Integration

Every AddOn must define its own static library target.

Minimal example:

```cmake
add_library(engine_myaddon STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/MyAddon.cpp
)

target_include_directories(engine_myaddon
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_link_libraries(engine_myaddon PUBLIC engine_core)
```

### Rules

- AddOn targets must link against `engine_core` if they use core engine APIs.
- AddOn-specific include directories must be exported through the target.
- All AddOn targets must include `${CMAKE_SOURCE_DIR}` (project root) as a PUBLIC include so that cross-addon includes of the form `"addons/<name>/..."` resolve correctly.
- Backend-specific system dependencies must stay in the AddOn target, not in `engine_core`.
- The root `CMakeLists.txt` includes AddOns via `add_subdirectory(addons/<name>)`.

### Important

`engine_core` must not link any AddOn library — not even as a convenience.
AddOn libraries belong in `engine_addon_adapters` or in the application target.
Violating this rule creates hidden circular dependencies between core and concrete implementations.

---

## Registration Models

KROM uses three registration models depending on AddOn type.

---

### 1. IEngineAddon — lifecycle registration (primary model)

This is the standard registration model for AddOns that need to participate in engine startup and shutdown.

Relevant engine contract:

- `engine::IEngineAddon` — `include/core/IEngineAddon.hpp`
- `engine::AddonContext` — `include/core/AddonContext.hpp`
- `engine::ServiceRegistry` — `include/core/ServiceRegistry.hpp`
- `engine::AddonManager` — `include/core/AddonManager.hpp`

#### AddonContext

`AddonContext` is passed to `Register()` and `Unregister()`. It exposes only stable core services:

```cpp
struct AddonContext
{
    ILogger&         Logger;
    events::EventBus& EventBus;
    ServiceRegistry& Services;
    void*            Config    = nullptr;
    void*            Allocator = nullptr;
};
```

AddOns must not assume any service is present. Use `Services.TryGet<T>()` for optional services and `Services.Has<T>()` for guards.

#### ServiceRegistry

Non-owning registry keyed by type. Services must be registered before `RegisterAll()` is called.

```cpp
services.Register<ecs::ComponentMetaRegistry>(&componentRegistry);
services.Register<renderer::RenderSystem>(&renderSystem);

auto* rs = ctx.Services.TryGet<renderer::RenderSystem>(); // nullptr if absent
auto* cm = ctx.Services.Get<ecs::ComponentMetaRegistry>(); // asserts if absent
bool  ok = ctx.Services.Has<renderer::RenderSystem>();
```

#### AddonManager

Manages AddOn lifetime. Registers in insertion order, unregisters in reverse order.

```cpp
AddonManager manager;
manager.AddAddon(std::make_unique<MyAddon>()); // rejected if name is duplicate or empty
manager.AddAddon(CreateCameraAddon());

AddonContext ctx{ GetDefaultLogger(), eventBus, services };
manager.RegisterAll(ctx);   // calls Register() forward; rolls back on failure
// ...
manager.UnregisterAll(ctx); // calls Unregister() in reverse
manager.Reset();            // clears all entries
```

If `Register()` fails, `UnregisterAll()` is called immediately including the failing AddOn, so partial teardown is attempted.

#### IEngineAddon pattern

```cpp
#pragma once
#include "core/IEngineAddon.hpp"

class MyAddon final : public engine::IEngineAddon
{
public:
    [[nodiscard]] const char*      Name()    const noexcept override { return "MyAddon"; }
    [[nodiscard]] std::string_view Version() const noexcept override { return "1.0.0"; }

    bool Register(engine::AddonContext& ctx) override
    {
        // Query services — never assume they are present.
        auto* components = ctx.Services.TryGet<engine::ecs::ComponentMetaRegistry>();
        if (!components)
        {
            ctx.Logger.Error("MyAddon requires ComponentMetaRegistry");
            return false;
        }
        engine::ecs::RegisterComponent<MyComponent>(*components, "MyComponent");
        return true;
    }

    void Unregister(engine::AddonContext& ctx) override
    {
        if (auto* components = ctx.Services.TryGet<engine::ecs::ComponentMetaRegistry>())
            components->Unregister<MyComponent>();
    }
};
```

---

### 2. Backend self-registration

Backend AddOns register themselves through `DeviceFactory::Registrar`.

Relevant engine contract:

- `engine::renderer::DeviceFactory`
- `engine::renderer::IDevice`

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

#### Backend registration rules

- registration must happen inside the AddOn
- the core must not manually know backend implementation classes
- the chosen `BackendType` must match the actual backend
- enumeration is optional but strongly recommended for real hardware backends

---

### 3. Feature registration

Feature AddOns expose `IEngineFeature` implementations. In practice these are always wrapped in an `IEngineAddon` adapter. Direct registration outside an adapter is only for test and tooling code.

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

#### Feature registration rules

- feature IDs must be stable and unique
- dependencies must be declared through `GetDependencies()` when needed
- `Register(...)` must only register feature-owned pipeline and extraction objects
- `Initialize(...)` must not assume unrelated AddOns are present unless declared as dependencies
- `Shutdown(...)` must leave no persistent registration or dangling ownership behind

---

## Combined AddOn Entry Points

When an AddOn registers both ECS components and serialization handlers, it must expose a combined entry point pair so that adapters and callers do not need to know the internal breakdown.

Convention:

```cpp
// In the AddOn's serialization header:
void RegisterMyAddon(ecs::ComponentMetaRegistry* components,
                     serialization::SceneSerializer* serializer,
                     serialization::SceneDeserializer* deserializer);

void UnregisterMyAddon(ecs::ComponentMetaRegistry* components,
                       serialization::SceneSerializer* serializer,
                       serialization::SceneDeserializer* deserializer);
```

All parameters are pointers. A null value means the service is not available — the function silently skips that step. Unregister must mirror Register in reverse order (deserializer, serializer, components).

This is the only function an `IEngineAddon` adapter needs to call for lifecycle setup beyond the render feature factory.

---

## Scene Extraction and Pipeline Extension

Feature AddOns are the approved place for render pipeline composition.

They may contribute:

- scene extraction steps via `ISceneExtractionStep`
- render pipelines via `IRenderPipeline`

### Scene extraction step contract

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

Typical examples: material descriptors, helper builders, shader parameter packing helpers.

### Rules

- helper AddOns must build as separate targets
- helper AddOns must not inject hardcoded policy into the core
- helper AddOns must not silently replace core abstractions
- app targets that use helper headers must link the helper AddOn target

---

## Linking Rules for Applications

An application must explicitly link the AddOns it uses.

```cmake
target_link_libraries(my_app PRIVATE
    engine_core
    engine_addon_adapters
    engine_forward
    engine_pbr
    engine_platform_win32
)
```

`engine_addon_adapters` transitively exposes `engine_camera`, `engine_lighting`, `engine_mesh_renderer`, `engine_particles`, and `engine_shadow`. Applications that use these do not need to list them separately as long as they link `engine_addon_adapters`.

If an application directly includes AddOn headers that are not covered transitively, it must add an explicit link.

---

## Self-Registering Static Libraries

Some AddOns rely on static initialization for registration (backend AddOns).
When linking static libraries, the linker may discard unreferenced object files.

KROM provides a helper:

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

Usage:

```cmake
krom_link_self_registering_addon(my_app engine_dx11)
```

If an AddOn depends on static initialization for registration, it must be linked this way.

---

## Include and Dependency Policy

### Core may depend on

- public engine headers under `include/`
- standard library
- shared abstractions and registries

### Core must not depend on

- backend AddOn headers
- feature AddOn implementation headers
- material/utility AddOn headers
- any concrete AddOn-private type

`engine_core` must not list any AddOn library in `target_link_libraries`. This includes `engine_camera`, `engine_lighting`, `engine_mesh_renderer`, and `engine_particles`. Those belong in `engine_addon_adapters`.

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

| Item | Convention | Example |
|---|---|---|
| folder | `addons/<name>` | `addons/forward` |
| CMake target | `engine_<name>` | `engine_forward` |
| namespace (renderer features) | `engine::renderer::addons::<name>` | `engine::renderer::addons::forward` |
| namespace (ECS/lifecycle addons) | `engine::addons::<name>` | `engine::addons::camera` |
| IEngineAddon name string | short descriptive word | `"Camera"`, `"MeshRenderer"` |
| IEngineFeature ID string | `krom-<name>` | `krom-forward` |

---

## Minimal IEngineAddon Example

This example shows how to create a complete lifecycle AddOn without a render feature.

### File layout

```text
addons/mylifecycle/
    CMakeLists.txt
    MyLifecycleSerialization.hpp
    MyLifecycleSerialization.cpp
```

### Combined entry point header

```cpp
#pragma once
#include "ecs/ComponentMeta.hpp"
#include "serialization/SceneSerializer.hpp"

namespace engine::addons::mylifecycle {

void RegisterMyLifecycleAddon(ecs::ComponentMetaRegistry* components,
                               serialization::SceneSerializer* serializer,
                               serialization::SceneDeserializer* deserializer);

void UnregisterMyLifecycleAddon(ecs::ComponentMetaRegistry* components,
                                 serialization::SceneSerializer* serializer,
                                 serialization::SceneDeserializer* deserializer);

} // namespace engine::addons::mylifecycle
```

### Combined entry point implementation

```cpp
#include "MyLifecycleSerialization.hpp"
#include "MyLifecycleComponents.hpp"

namespace engine::addons::mylifecycle {

void RegisterMyLifecycleAddon(ecs::ComponentMetaRegistry* components,
                               serialization::SceneSerializer* serializer,
                               serialization::SceneDeserializer* deserializer)
{
    if (components)  RegisterMyLifecycleComponents(*components);
    if (serializer)  RegisterMyLifecycleSerializationHandlers(*serializer);
    if (deserializer) RegisterMyLifecycleDeserializationHandlers(*deserializer);
}

void UnregisterMyLifecycleAddon(ecs::ComponentMetaRegistry* components,
                                 serialization::SceneSerializer* serializer,
                                 serialization::SceneDeserializer* deserializer)
{
    if (deserializer) UnregisterMyLifecycleDeserializationHandlers(*deserializer);
    if (serializer)  UnregisterMyLifecycleSerializationHandlers(*serializer);
    if (components)  components->Unregister<MyLifecycleComponent>();
}

} // namespace engine::addons::mylifecycle
```

### IEngineAddon adapter

```cpp
class MyLifecycleAddon final : public engine::IEngineAddon
{
public:
    [[nodiscard]] const char* Name() const noexcept override { return "MyLifecycle"; }

    bool Register(engine::AddonContext& ctx) override
    {
        if (!ctx.Services.Has<engine::ecs::ComponentMetaRegistry>())
        {
            ctx.Logger.Error("MyLifecycleAddon requires ComponentMetaRegistry");
            return false;
        }
        engine::addons::mylifecycle::RegisterMyLifecycleAddon(
            ctx.Services.TryGet<engine::ecs::ComponentMetaRegistry>(),
            ctx.Services.TryGet<engine::serialization::SceneSerializer>(),
            ctx.Services.TryGet<engine::serialization::SceneDeserializer>());
        return true;
    }

    void Unregister(engine::AddonContext& ctx) override
    {
        engine::addons::mylifecycle::UnregisterMyLifecycleAddon(
            ctx.Services.TryGet<engine::ecs::ComponentMetaRegistry>(),
            ctx.Services.TryGet<engine::serialization::SceneSerializer>(),
            ctx.Services.TryGet<engine::serialization::SceneDeserializer>());
    }
};
```

### Bootstrap

```cpp
engine::ServiceRegistry services;
services.Register<engine::ecs::ComponentMetaRegistry>(&componentRegistry);
// renderer::RenderSystem registered here if render features are needed

engine::AddonManager manager;
manager.AddAddon(std::make_unique<MyLifecycleAddon>());

engine::AddonContext ctx{ engine::GetDefaultLogger(), eventBus, services };
manager.RegisterAll(ctx);
// ... run ...
manager.UnregisterAll(ctx);
```

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
add_library(engine_sample_backend STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/SampleBackendDevice.cpp
)

target_include_directories(engine_sample_backend
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
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

    void Register(FeatureRegistrationContext& context) override { (void)context; }
    bool Initialize(const FeatureInitializationContext& context) override { (void)context; return true; }
    void Shutdown(const FeatureShutdownContext& context) override { (void)context; }
};

} // namespace

std::unique_ptr<IEngineFeature> CreateSampleFeature()
{
    return std::make_unique<SampleFeature>();
}

} // namespace engine::renderer::addons::sample_feature
```

### IEngineAddon adapter for a render feature

```cpp
class SampleFeatureAddon final : public engine::IEngineAddon
{
public:
    [[nodiscard]] const char* Name() const noexcept override { return "SampleFeature"; }

    bool Register(engine::AddonContext& ctx) override
    {
        auto* rs = ctx.Services.TryGet<engine::renderer::RenderSystem>();
        if (!rs)
        {
            ctx.Logger.Error("SampleFeatureAddon requires RenderSystem");
            return false;
        }
        return rs->RegisterFeature(
            engine::renderer::addons::sample_feature::CreateSampleFeature());
    }

    void Unregister(engine::AddonContext& ctx) override
    {
        // FeatureRegistry::RemoveFeature() not yet available.
        (void)ctx;
    }
};
```

---

## Known Limitations

### FeatureRegistry has no RemoveFeature

`IEngineFeature` instances registered via `RenderSystem::RegisterFeature()` cannot be removed at runtime.
`Shutdown(...)` is called on feature shutdown but the feature stays in the registry.

Until `FeatureRegistry::RemoveFeature()` is added, `Unregister()` of render feature adapters is effectively a no-op for the feature itself.
Component and serialization state can be cleaned up — only the render feature slot persists.

### ComponentTypeID values are permanent

`ComponentTypeID<T>::value` is a compile-time static counter. Calling `ComponentMetaRegistry::Unregister<T>()` zeroes the metadata slot so `Get<T>()` returns `nullptr`, but the ID value itself is never reclaimed. Do not rely on type ID values being stable across rebuilds that add or remove component types.

---

## What an AddOn Must Not Do

An AddOn must not:

- modify core invariants from the outside
- require the core to include AddOn-private headers
- hide registration in unrelated engine code
- create silent side dependencies on unrelated AddOns
- hardcode assumptions that belong in content or app bootstrap
- bypass engine abstraction layers without a clearly documented reason
- be listed in `engine_core`'s `target_link_libraries`

---

## Validation Checklist

Before calling a module an AddOn, verify all of the following:

- it lives in `addons/<name>/`
- it has its own `CMakeLists.txt`
- it builds as a dedicated target
- its CMake target exports `${CMAKE_SOURCE_DIR}` as a PUBLIC include
- it exposes a clear public API
- it has a documented registration path (IEngineAddon, backend registrar, or feature factory)
- it does not require core-private hacks
- `engine_core` does not list it in its own `target_link_libraries`
- app targets explicitly link it when they use it
- its ownership and lifecycle are clear
- `Unregister()` undoes everything `Register()` did, in reverse order

---

## Recommended Extension Workflow

When creating a new AddOn:

1. Define the AddOn category (backend / feature / utility / lifecycle).
2. Define the public API surface — prefer narrow headers.
3. Create the AddOn build target with correct include directories.
4. Implement the combined entry point (`RegisterXxxAddon` / `UnregisterXxxAddon`) if the AddOn touches components or serialization.
5. Implement `IEngineAddon` adapter calling the entry point and/or feature factory.
6. Integrate into the root build via `add_subdirectory`.
7. Add the AddOn to `engine_addon_adapters` if it should be part of the standard engine stack.
8. Link explicitly from example or app targets.
9. Verify that no core code now depends on AddOn-private details.

---

## Final Rule

KROM AddOns are not cosmetic folders.
They are architectural boundaries.

A proper AddOn is:

- isolated
- buildable
- explicit
- registerable
- reversible

If those properties are missing, the design has not been finished.
