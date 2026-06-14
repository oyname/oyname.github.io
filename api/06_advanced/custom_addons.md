# Custom AddOns

`engine::renderer::IEngineFeature` · `engine::renderer::IRenderPipeline`

AddOns extend the engine without modifying Core. A feature registers scene extraction steps and render pipelines through a `FeatureRegistry`. Core never references AddOn code — the dependency always points inward.

---

## What an AddOn is

An AddOn is a CMake library that links against `engine_core` and registers itself at startup via a static initializer. It can contribute:

- a **render pipeline** — a `Build()` function that adds passes to the RenderGraph
- a **scene extraction step** — a function that reads ECS data and fills the `SceneSnapshot`
- both of the above

---

## Implementing IEngineFeature

```cpp
#include "renderer/FeatureRegistry.hpp"

class MyFeature final : public engine::renderer::IEngineFeature {
public:
    std::string_view GetName() const noexcept override { return "MyFeature"; }
    FeatureID        GetID()   const noexcept override { return FeatureID{ 0xMY_HASH }; }

    void Register(FeatureRegistrationContext& ctx) override {
        ctx.RegisterRenderPipeline(
            std::make_shared<MyRenderPipeline>(), /*makeDefault=*/true);
        ctx.RegisterSceneExtractionStep(
            std::make_shared<MyExtractionStep>());
    }

    bool Initialize(const FeatureInitializationContext& ctx) override {
        // compile shaders, create GPU resources
        return true;
    }

    void Shutdown(const FeatureShutdownContext& ctx) override {
        // release GPU resources
    }
};
```

---

## Implementing IRenderPipeline

```cpp
class MyRenderPipeline final : public engine::renderer::IRenderPipeline {
public:
    std::string_view GetName() const noexcept override { return "MyPipeline"; }

    bool Build(const RenderPipelineBuildContext& ctx,
               RenderPipelineBuildResult& result) const override
    {
        auto& rg = ctx.renderGraph;

        RGResourceID backbuffer = rg.ImportBackbuffer(
            ctx.backbufferRT, ctx.backbufferTex,
            ctx.viewportWidth, ctx.viewportHeight);

        rg.AddPass("MyOpaquePass")
            .WriteRenderTarget(backbuffer)
            .Execute([&](const RGExecContext& exec) {
                // draw opaque geometry
            });

        return true;
    }
};
```

The `RenderPipelineBuildContext` provides everything the pipeline needs: the RenderGraph, backbuffer handles, viewport size, `ShaderRuntime`, `MaterialSystem`, and the per-frame constant buffer.

---

## Implementing ISceneExtractionStep

```cpp
class MyExtractionStep final : public engine::renderer::ISceneExtractionStep {
public:
    std::string_view GetName() const noexcept override { return "MyExtraction"; }

    void Extract(const SceneExtractionContext& ctx) const override {
        // Read ECS data via ctx and fill the scene snapshot.
        (void)ctx;
    }
};
```

---

## Registering the feature

Features are registered directly with `RenderSystem` before the render loop is initialized. The standard pattern is to expose a `CreateMyFeature()` factory function from the AddOn and call `RegisterFeature` from the application or example bootstrap code:

```cpp
// In ExampleApp / main — before Initialize():
renderer::RenderSystem& renderSystem = m_renderLoop.GetRenderSystem();
renderSystem.RegisterFeature(std::make_unique<MyFeature>());
```

Multiple features can be registered in sequence:

```cpp
renderer::RenderSystem& renderSystem = m_renderLoop.GetRenderSystem();
renderSystem.RegisterFeature(addons::mesh_renderer::CreateMeshRendererFeature());
renderSystem.RegisterFeature(addons::lighting::CreateLightingFeature());
renderSystem.RegisterFeature(addons::shadow::CreateShadowFeature());
renderSystem.RegisterFeature(std::make_unique<MyFeature>());
```

There is no `GlobalFeatureRegistry` or static auto-registration for feature AddOns. Registration always happens explicitly in application bootstrap code.

---

## CMake integration

```cmake
add_library(engine_myfeature STATIC
    MyFeature.cpp
)

target_link_libraries(engine_myfeature PUBLIC engine_core)
target_compile_definitions(engine_myfeature PUBLIC KROM_MYFEATURE)

# Link the feature AddOn to the application target:
target_link_libraries(my_app PRIVATE engine_myfeature)
```

Feature AddOns do not use static auto-registration, so the `krom_link_self_registering_addon` helper is not required here. That helper is only needed for backend AddOns that register themselves through `DeviceFactory::Registrar`.

---

## Feature dependencies

```cpp
std::vector<FeatureID> GetDependencies() const noexcept override {
    return { ForwardFeatureID };
}
```

`FeatureRegistry` topologically sorts features before calling `Initialize`. A feature's `Initialize` is always called after all its declared dependencies have been initialized.

---

## Core boundary rules

- AddOns may include `engine_core` headers.
- Core must never include AddOn headers.
- Backend names must not appear as strings or `#ifdef` symbols in Core.
- A new backend or feature is always a new AddOn library — never a change to `engine_core`.
