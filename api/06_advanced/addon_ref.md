# KROM Engine AddOn API Reference

## Purpose

This document defines the AddOn model for KROM Engine.
It is intentionally general and should be used as the baseline reference for anyone who wants to create new engine extensions.

An AddOn is a self-contained module that extends the engine **without modifying the core contract**.
AddOns are the approved extension mechanism for:

- rendering backends
- render features and pipelines
- ECS components and serialization
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
- **Dependencies are declared, not discovered at runtime.**
- **No hidden coupling between core and individual AddOns.**

If a module has no isolated build target, no clear public API, and no declared dependency contract, then it is not a real AddOn.

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
AddOn libraries belong in the application target or a purpose-specific aggregate target.
Violating this rule creates hidden circular dependencies between core and concrete implementations.

---

## Registration Models

KROM uses three registration models depending on AddOn type.

---

### 1. IEngineModule — lifecycle registration (primary model)

This is the standard registration model for AddOns that need to participate in engine startup and shutdown, register ECS components, contribute render capabilities, or provide services to other modules.

Relevant engine contract:

- `engine::modules::IEngineModule` — `include/core/ModuleSystem.hpp`
- `engine::modules::ModuleDescriptor` — `include/core/ModuleSystem.hpp`
- `engine::modules::ModuleDeclarationContext` — `include/core/ModuleSystem.hpp`
- `engine::modules::ModuleInitContext` — `include/core/ModuleSystem.hpp`
- `engine::modules::ModuleShutdownContext` — `include/core/ModuleSystem.hpp`
- `engine::modules::ModuleCatalog` — `include/core/ModuleSystem.hpp`
- `engine::modules::ModuleGraph` — `include/core/ModuleSystem.hpp`
- `engine::modules::ModuleRuntime` — `include/core/ModuleSystem.hpp`
- `engine::modules::ServiceContainer` — `include/core/ModuleSystem.hpp`

#### IEngineModule interface

```cpp
class IEngineModule
{
public:
    virtual ~IEngineModule() = default;

    [[nodiscard]] virtual ModuleDescriptor Describe() const noexcept = 0;
    virtual void DeclareContributions(ModuleDeclarationContext& ctx) const = 0;
    virtual bool Initialize(ModuleInitContext& ctx) = 0;
    virtual void Shutdown(ModuleShutdownContext& ctx) noexcept = 0;
};
```

The four methods map to four distinct phases. Their invariants are strict:

| Method | Phase | Side effects allowed |
|---|---|---|
| `Describe()` | metadata | none — must be side-effect-free and `const` |
| `DeclareContributions()` | declaration | none — write-only into context, no service access |
| `Initialize()` | initialization | yes — may access declared services, register runtime state |
| `Shutdown()` | shutdown | yes — must reverse Initialize() |

#### ModuleDescriptor

```cpp
struct ModuleDescriptor
{
    std::string_view              name{};
    RawModuleId                   id{};
    std::span<const RawModuleId>  requiredModules{};
    std::span<const RawModuleId>  optionalModules{};
    std::span<const RawServiceId> requiredServices{};
    std::span<const RawServiceId> optionalServices{};
};
```

All dependency declarations use `std::string_view` / `std::span` over static storage. The engine interns them at catalog-add time — no ownership required from the module.

#### ServiceContainer

Typed, tier-separated service registry. Has no `TryGet` or free lookup.
Modules may only access services they declared in `Describe()`.

```cpp
// Tier-0: registered by the engine before any module loads
services.RegisterCore<ecs::ComponentMetaRegistry>("engine.component_registry", componentRegistry);
services.RegisterCore<renderer::RenderSystem>("engine.render_system",           renderSystem);

// In Initialize(): declared-required service
auto& components = ctx.GetRequired<ecs::ComponentMetaRegistry>();  // via ServiceTraits

// In Initialize(): declared-optional service
auto* debug = ctx.GetOptional<IDebugOverlay>();                    // nullptr if absent
```

`GetRequired` throws if the service is missing or has a type mismatch.
`GetOptional` returns `nullptr` if absent, throws on type mismatch.
Accessing a service that was not declared in `Describe()` throws unconditionally.

#### ServiceTraits

To use the type-based lookup form (`GetRequired<T>()` without an explicit ID), define a specialization:

```cpp
template<>
struct engine::modules::ServiceTraits<ecs::ComponentMetaRegistry>
{
    static constexpr auto kId = "engine.component_registry";
};
```

Core engine services have `ServiceTraits` defined in their headers. Third-party services without traits must be accessed via the explicit `GetRequired<T>(ServiceId)` overload.

#### Service tiers

| Tier | Who registers | When available |
|---|---|---|
| Core (`ServiceTier::Core`) | `EngineRuntime` before module graph | before any module loads |
| Module (`ServiceTier::Module`) | module in `Initialize()` via `ctx.RegisterProvidedService()` | after the providing module initializes |

A module-provided service is only available to modules that declared it as a dependency. Core services cannot be shadowed by modules.

#### IEngineModule pattern

```cpp
#pragma once
#include "core/ModuleSystem.hpp"

class MyLifecycleModule final : public engine::modules::IEngineModule
{
public:
    [[nodiscard]] engine::modules::ModuleDescriptor Describe() const noexcept override
    {
        static const engine::modules::RawServiceId kRequired[] = {
            "engine.component_registry",
            "engine.scene_serializer",
        };
        static const engine::modules::RawServiceId kOptional[] = {
            "engine.scene_deserializer",
        };
        return {
            .name             = "addon.mylifecycle",
            .id               = "addon.mylifecycle",
            .requiredServices = kRequired,
            .optionalServices = kOptional,
        };
    }

    void DeclareContributions(engine::modules::ModuleDeclarationContext& ctx) const override
    {
        // No contributions for a lifecycle-only module.
        (void)ctx;
    }

    bool Initialize(engine::modules::ModuleInitContext& ctx) override
    {
        auto& components  = ctx.GetRequired<ecs::ComponentMetaRegistry>();
        auto& serializer  = ctx.GetRequired<serialization::SceneSerializer>();
        auto* deserializer = ctx.GetOptional<serialization::SceneDeserializer>();

        engine::ecs::RegisterComponent<MyComponent>(components, "MyComponent");
        RegisterMySerializationHandlers(serializer);
        if (deserializer)
            RegisterMyDeserializationHandlers(*deserializer);

        return true;
    }

    void Shutdown(engine::modules::ModuleShutdownContext& ctx) noexcept override
    {
        // Reverse Initialize() in reverse order.
        if (auto* deserializer = ctx.GetOptional<serialization::SceneDeserializer>(...))
            UnregisterMyDeserializationHandlers(*deserializer);
        // serializer and components cleaned up analogously
    }
};
```

#### Bootstrap

```cpp
// 1. Register Tier-0 services.
engine::modules::ServiceContainer services;
services.RegisterCore<ecs::ComponentMetaRegistry>("engine.component_registry", componentRegistry);
services.RegisterCore<serialization::SceneSerializer>("engine.scene_serializer", sceneSerializer);
services.RegisterCore<serialization::SceneDeserializer>("engine.scene_deserializer", sceneDeserializer);

// 2. Add modules to catalog.
engine::modules::ModuleCatalog catalog;
catalog.AddModule(std::make_unique<MyLifecycleModule>());

// 3. Declare contributions (side-effect-free).
if (!catalog.DeclareAll())
{ /* handle catalog.GetLastError() */ }

// 4. Build and validate dependency graph.
engine::modules::ModuleGraph graph;
if (!graph.Build(catalog, services.DescribeServices()))
{ /* handle graph.GetLastError() */ }

// 5. Initialize in topological order.
engine::modules::ModuleRuntime runtime;
if (!runtime.InitializeAll(catalog, graph, services))
{ /* handle runtime.GetLastError() */ }

// ... run ...

// 6. Shutdown in reverse topological order.
runtime.ShutdownAll(catalog, services);
```

If `Build()` fails, no module has been initialized — no rollback needed.
If `InitializeAll()` fails, already-initialized modules are shut down automatically in reverse order.

---

### 2. Backend self-registration

Backend AddOns register themselves through `DeviceFactory::Registrar`.
This model is unchanged.

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

Feature AddOns expose `IEngineFeature` implementations and register them directly with the render system. No adapter layer is needed.

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

Direct registration from app or example code:

```cpp
renderer::RenderSystem& renderSystem = m_renderLoop.GetRenderSystem();
renderSystem.RegisterFeature(addons::mesh_renderer::CreateMeshRendererFeature());
renderSystem.RegisterFeature(addons::lighting::CreateLightingFeature());
renderSystem.RegisterFeature(addons::shadow::CreateShadowFeature());
renderSystem.RegisterFeature(renderer::addons::forward::CreateForwardFeature(forwardConfig));
```

#### Feature registration rules

- feature IDs must be stable and unique
- dependencies must be declared through `GetDependencies()` when needed
- `Register(...)` must only register feature-owned pipeline and extraction objects
- `Initialize(...)` must not assume unrelated features are present unless declared as dependencies
- `Shutdown(...)` must leave no persistent registration or dangling ownership behind

---

## Contributions

Modules may contribute typed objects to engine subsystems by declaring them in `DeclareContributions()`.
Contributions are type-erased at declaration time and instantiated on demand through `ContributionRegistry<T>`.

### Declaring a contribution

```cpp
void DeclareContributions(engine::modules::ModuleDeclarationContext& ctx) const override
{
    ctx.ContributeCapability<IMyCapability>({
        .name    = "addon.mymodule.capability",
        .factory = []() -> std::unique_ptr<IMyCapability> {
            return std::make_unique<MyCapabilityImpl>();
        }
    });
}
```

- `DeclareContributions()` is `const` — no side effects, no service access.
- The factory is called later during initialization, not at declaration time.
- The type is bound at registration, not at instantiation. Type mismatches throw at `Instantiate()`.

### Consuming contributions

After `DeclareAll()`, any subsystem or runtime can collect contributions by type:

```cpp
engine::modules::ContributionRegistry<IMyCapability> registry(catalog);

// Inspect declarations without instantiation:
for (size_t i = 0; i < registry.Size(); ++i)
    std::cout << registry.Declaration(i).name.View() << "\n";

// Instantiate all at once:
auto instances = registry.InstantiateAll();

// Instantiate individually:
auto instance = registry.Instantiate(0);
```

`ContributionRegistry<T>` queries the catalog at construction and holds non-owning pointers.
It must not outlive the `ModuleCatalog` it was built from.

---

## Scene Extraction and Pipeline Extension

Feature AddOns are the approved place for render pipeline composition.

They may contribute:

- scene extraction steps via `ISceneExtractionStep`
- render pipelines via `IRenderPipeline`
- frame constants via `IFrameConstantsContributor`

### Scene extraction step contract

```cpp
class MyExtractionStep final : public ISceneExtractionStep
{
public:
    std::string_view GetName() const noexcept override { return "myfeature.extract"; }

    void Extract(const SceneExtractionContext& ctx) const override
    {
        (void)ctx;
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
    engine_forward
    engine_pbr
    engine_platform_win32
)
```

There is no aggregate adapter library. Each AddOn is linked directly.
If an application includes AddOn headers, it must link the corresponding target.

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

`engine_core` must not list any AddOn library in `target_link_libraries`.

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
| module ID string | `addon.<name>` | `addon.mesh_renderer` |
| service ID string | `engine.<name>` (core) / `addon.<name>` (module) | `engine.render_system` |
| `IEngineFeature` ID string | `krom-<name>` | `krom-forward` |

---

## Minimal IEngineModule Example

This example shows how to create a complete lifecycle module without a render feature.

### File layout

```text
addons/mylifecycle/
    CMakeLists.txt
    MyLifecycleComponents.hpp
    MyLifecycleComponents.cpp
    MyLifecycleSerialization.hpp
    MyLifecycleSerialization.cpp
    MyLifecycleModule.hpp
    MyLifecycleModule.cpp
```

### Module header

```cpp
#pragma once
#include "core/ModuleSystem.hpp"

namespace engine::addons::mylifecycle {

class MyLifecycleModule final : public engine::modules::IEngineModule
{
public:
    [[nodiscard]] engine::modules::ModuleDescriptor Describe() const noexcept override;
    void DeclareContributions(engine::modules::ModuleDeclarationContext& ctx) const override;
    bool Initialize(engine::modules::ModuleInitContext& ctx) override;
    void Shutdown(engine::modules::ModuleShutdownContext& ctx) noexcept override;
};

} // namespace engine::addons::mylifecycle
```

### Module implementation

```cpp
#include "MyLifecycleModule.hpp"
#include "MyLifecycleComponents.hpp"
#include "MyLifecycleSerialization.hpp"
#include "ecs/ComponentMeta.hpp"
#include "serialization/SceneSerializer.hpp"

namespace engine::addons::mylifecycle {

engine::modules::ModuleDescriptor MyLifecycleModule::Describe() const noexcept
{
    static const engine::modules::RawServiceId kRequired[] = {
        "engine.component_registry",
        "engine.scene_serializer",
    };
    static const engine::modules::RawServiceId kOptional[] = {
        "engine.scene_deserializer",
    };
    return {
        .name             = "addon.mylifecycle",
        .id               = "addon.mylifecycle",
        .requiredServices = kRequired,
        .optionalServices = kOptional,
    };
}

void MyLifecycleModule::DeclareContributions(engine::modules::ModuleDeclarationContext& ctx) const
{
    (void)ctx; // no contributions for this module
}

bool MyLifecycleModule::Initialize(engine::modules::ModuleInitContext& ctx)
{
    auto& components   = ctx.GetRequired<ecs::ComponentMetaRegistry>();
    auto& serializer   = ctx.GetRequired<serialization::SceneSerializer>();
    auto* deserializer = ctx.GetOptional<serialization::SceneDeserializer>();

    RegisterMyLifecycleComponents(components);
    RegisterMyLifecycleSerializationHandlers(serializer);
    if (deserializer)
        RegisterMyLifecycleDeserializationHandlers(*deserializer);

    return true;
}

void MyLifecycleModule::Shutdown(engine::modules::ModuleShutdownContext& ctx) noexcept
{
    auto* deserializer = ctx.GetOptional<serialization::SceneDeserializer>(...);
    auto* serializer   = ctx.GetOptional<serialization::SceneSerializer>(...);
    auto* components   = ctx.GetOptional<ecs::ComponentMetaRegistry>(...);

    if (deserializer) UnregisterMyLifecycleDeserializationHandlers(*deserializer);
    if (serializer)   UnregisterMyLifecycleSerializationHandlers(*serializer);
    if (components)   components->Unregister<MyLifecycleComponent>();
}

} // namespace engine::addons::mylifecycle
```

### Bootstrap

```cpp
engine::modules::ServiceContainer services;
services.RegisterCore<ecs::ComponentMetaRegistry>("engine.component_registry", componentRegistry);
services.RegisterCore<serialization::SceneSerializer>("engine.scene_serializer", sceneSerializer);
services.RegisterCore<serialization::SceneDeserializer>("engine.scene_deserializer", sceneDeserializer);

engine::modules::ModuleCatalog catalog;
catalog.AddModule(std::make_unique<engine::addons::mylifecycle::MyLifecycleModule>());

catalog.DeclareAll();

engine::modules::ModuleGraph graph;
graph.Build(catalog, services.DescribeServices());

engine::modules::ModuleRuntime runtime;
runtime.InitializeAll(catalog, graph, services);
// ... run ...
runtime.ShutdownAll(catalog, services);
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

### Registration from app code

```cpp
renderSystem.RegisterFeature(
    engine::renderer::addons::sample_feature::CreateSampleFeature());
```

---

## Known Limitations

### ComponentTypeID values are permanent

`ComponentTypeID<T>::value` is a compile-time static counter. Calling `ComponentMetaRegistry::Unregister<T>()` zeroes the metadata slot so `Get<T>()` returns `nullptr`, but the ID value itself is never reclaimed. Do not rely on type ID values being stable across rebuilds that add or remove component types.

---

## What a Module Must Not Do

A module must not:

- access services that were not declared in `Describe()`
- cause side effects in `Describe()` or `DeclareContributions()`
- read other modules' declarations during `DeclareContributions()`
- register Tier-0 services — only the engine runtime may do that
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
- it implements `IEngineModule` or uses backend self-registration or direct feature registration
- all required and optional dependencies are declared in `Describe()`
- `DeclareContributions()` is side-effect-free and accesses no services
- `Shutdown()` reverses `Initialize()` in reverse order
- it does not require core-private hacks
- `engine_core` does not list it in its own `target_link_libraries`
- app targets explicitly link it when they use it

---

## Recommended Extension Workflow

When creating a new AddOn:

1. Define the AddOn category (backend / feature / utility / lifecycle).
2. Define the public API surface — prefer narrow headers.
3. Create the AddOn build target with correct include directories.
4. Implement `IEngineModule`:
   - declare all required and optional services in `Describe()`
   - declare all contributions in `DeclareContributions()` (side-effect-free)
   - access only declared services in `Initialize()`
   - reverse `Initialize()` in `Shutdown()`
5. If the AddOn provides render features, expose a `CreateXxxFeature()` factory.
6. Integrate into the root build via `add_subdirectory`.
7. Link explicitly from example or app targets.
8. Verify that no core code now depends on AddOn-private details.

---

## Final Rule

KROM AddOns are not cosmetic folders.
They are architectural boundaries.

A proper AddOn is:

- isolated
- buildable
- explicit in its dependencies
- validated before initialization
- reversible on shutdown

If those properties are missing, the design has not been finished.
