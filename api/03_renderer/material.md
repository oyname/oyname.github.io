# Material

`engine::renderer::MaterialSystem` · `engine::renderer::MaterialDesc` · `engine::renderer::MaterialHandle`

Materials describe **what is rendered**, **how it is shaded**, and **which fixed pipeline state is required**. A material combines domain and surface classification, feature flags, shader references, vertex layout, render target formats, depth/blend/rasterizer state, named parameters, texture slots, and render policy.

`MaterialSystem` stores and manages registered material descriptions. A material can be used directly or cloned as an instance. Instances inherit the base material setup and can override parameter values without changing the underlying pipeline as long as the pipeline-relevant state stays identical.

Materials are not registered directly through `MaterialSystem::RegisterMaterial`. The correct entry point for full material creation is `MaterialRuntimeBridge::RegisterMaterial`, which combines a `MaterialDesc` with a `MaterialRuntimeDesc` and returns a `MaterialHandle`.

---

## What belongs to a MaterialDesc

`MaterialDesc` defines the logical, backend-neutral material description.

Fields:

| Field | Type | Meaning |
|---|---|---|
| `name` | `std::string` | Human-readable identifier |
| `surface` | `MaterialSurfaceType` | Surface behavior: `Opaque`, `Transparent`, `Cutout`, `Unlit`, `ShadowOnly` |
| `features` | `MaterialFeatureFlags` | Bitmask of active material features |
| `domain` | `MaterialDomain` | Where the material is used: `Mesh`, `Postprocess`, `UI`, `Shadow`, `DepthOnly`, `Debug` |
| `parameters` | `vector<MaterialParam>` | Named typed parameters (float, vec4, texture, sampler) |
| `textureSlots` | `vector<MaterialTextureSlot>` | Named texture bindings with semantic annotations |
| `parameterLayout` | — | CB layout descriptor derived from parameter list |
| `renderPolicy` | `MaterialRenderPolicy` | Engine-level policy: culling, shadow casting/receiving |
| `sortLayer` | `uint32_t` | Sort layer for draw-order control |
| `materialGraph` | — | Optional shader graph descriptor |

---

## MaterialDomain

| Value | Usage |
|---|---|
| `MaterialDomain::Mesh` | Standard geometry rendering |
| `MaterialDomain::Postprocess` | Full-screen and post-processing passes |
| `MaterialDomain::UI` | Overlay and UI rendering |
| `MaterialDomain::Shadow` | Shadow map passes |
| `MaterialDomain::DepthOnly` | Depth-only prepass |
| `MaterialDomain::Debug` | Debug visualization passes |

---

## MaterialSurfaceType

| Value | Meaning |
|---|---|
| `MaterialSurfaceType::Opaque` | Fully opaque surface |
| `MaterialSurfaceType::Transparent` | Alpha-blended surface |
| `MaterialSurfaceType::Cutout` | Alpha-tested / masked surface |
| `MaterialSurfaceType::Unlit` | No lighting contribution |
| `MaterialSurfaceType::ShadowOnly` | Receives shadows but does not render visibly |

---

## MaterialFeatureFlags

Feature flags are combined with `|`. They control which shader permutation is selected and which parameter slots are active.

Commonly used flags:

| Flag | Meaning |
|---|---|
| `PbrMetalRough` | PBR metallic/roughness workflow |
| `NormalMap` | Normal mapping enabled |
| `BaseColorMap` | Albedo texture slot active |
| `MetallicMap` | Separate metallic texture |
| `RoughnessMap` | Separate roughness texture |
| `OcclusionMap` | Ambient occlusion texture |
| `EmissiveMap` | Emissive texture |
| `OrmMap` | Packed ORM texture (occlusion/roughness/metallic) |
| `IblMap` | IBL environment contribution enabled |
| `AlphaTest` | Alpha cutout test enabled |
| `DoubleSided` | Disables backface culling |
| `Unlit` | No lighting, output base color directly |
| `ShadowCaster` | Material participates in shadow pass |

---

## MaterialRenderPolicy

`MaterialRenderPolicy` contains engine-level rendering behavior.

Relevant fields:

| Field | Type | Meaning |
|---|---|---|
| `cull.mode` | `MaterialCullMode` | `Back`, `Front`, or `None` |
| `castShadows` | `bool` | Whether the material casts shadows |
| `receiveShadows` | `bool` | Whether the material receives shadows |

---

## MaterialParam helpers

Parameters are constructed with helper functions defined in the engine:

```cpp
MaterialParam MakeFloatParam(const char* name, float value);
MaterialParam MakeVec4Param(const char* name, math::Vec4 v);
MaterialParam MakeTextureParam(const char* name);
MaterialParam MakeSamplerParam(const char* name, uint32_t samplerIdx);
```

---

## MaterialRuntimeDesc

`MaterialRuntimeDesc` describes the GPU-side wiring: which shaders to use, which render pass to target, vertex layout, and render target formats.

```cpp
MaterialRuntimeDesc runtime{};
runtime.renderPass     = StandardRenderPasses::Opaque();
runtime.vertexShader   = vs;
runtime.fragmentShader = fs;
runtime.shadowShader   = shadow;
runtime.vertexLayout   = vertexLayout;
runtime.colorFormat    = Format::RGBA16_FLOAT;
runtime.depthFormat    = Format::D32_FLOAT;
```

Available `StandardRenderPasses` factories:

- `StandardRenderPasses::Opaque()`
- `StandardRenderPasses::Shadow()`
- `StandardRenderPasses::Postprocess()`

---

## Registering a material

Use `MaterialRuntimeBridge::RegisterMaterial` to combine a `MaterialDesc` with a `MaterialRuntimeDesc` and obtain a handle:

```cpp
MaterialParam MakeFloatParam(const char* name, float value) { ... }
MaterialParam MakeVec4Param(const char* name, math::Vec4 v) { ... }
MaterialParam MakeTextureParam(const char* name) { ... }
MaterialParam MakeSamplerParam(const char* name, uint32_t idx) { ... }

MaterialDesc desc{};
desc.name    = "MyMaterial";
desc.domain  = MaterialDomain::Mesh;
desc.surface = MaterialSurfaceType::Opaque;
desc.features = MaterialFeatureFlags::PbrMetalRough
              | MaterialFeatureFlags::NormalMap
              | MaterialFeatureFlags::BaseColorMap;

desc.renderPolicy.cull.mode      = MaterialCullMode::Back;
desc.renderPolicy.castShadows    = true;
desc.renderPolicy.receiveShadows = true;

desc.parameters = {
    MakeVec4Param ("baseColorFactor",  {1, 1, 1, 1}),
    MakeFloatParam("roughnessFactor",  0.5f),
    MakeFloatParam("metallicFactor",   0.0f),
    MakeFloatParam("normalStrength",   1.0f),
    MakeFloatParam("alphaCutoff",      0.5f),
    MakeTextureParam("albedo"),
    MakeTextureParam("normal"),
    MakeSamplerParam("sLinearWrap", SamplerSlots::LinearWrap),
};
desc.textureSlots = {
    { "albedo", MaterialTextureSemantic::BaseColor },
    { "normal", MaterialTextureSemantic::Normal    },
};

MaterialRuntimeDesc runtime{};
runtime.renderPass     = StandardRenderPasses::Opaque();
runtime.vertexShader   = vs;
runtime.fragmentShader = fs;
runtime.shadowShader   = shadow;
runtime.vertexLayout   = vertexLayout;
runtime.colorFormat    = Format::RGBA16_FLOAT;
runtime.depthFormat    = Format::D32_FLOAT;

MaterialHandle mat = MaterialRuntimeBridge::RegisterMaterial(
    materialSystem, std::move(desc), runtime);
```

---

## Creating instances

Material instances are derived from a registered base material.

```cpp
MaterialHandle inst = materials.CreateInstance(mat, "MyInstance");
materials.SetVec4   (inst, "baseColorFactor", {1.f, 0.2f, 0.1f, 1.f});
materials.SetFloat  (inst, "roughnessFactor", 0.8f);
materials.SetTexture(inst, "albedo",          gpuAlbedoTex);
materials.MarkDirty (inst);
```

Instances reuse the same pipeline state as long as no pipeline-relevant fields differ. Parameter variation does not automatically create a new PSO.

---

## Setting parameters

Named parameters can be updated through the material system.

```cpp
materials.SetFloat  (handle, "roughnessFactor", 0.4f);
materials.SetVec4   (handle, "baseColorFactor", {0.8f, 0.6f, 0.2f, 1.f});
materials.SetTexture(handle, "albedo",          texHandle);
materials.MarkDirty (handle);
```

Current behavior:

- `SetFloat` and `SetVec4` update parameter data and mark the material dirty automatically.
- `SetTexture` updates the texture binding, but an explicit `MarkDirty(handle)` may still be required depending on the path and backend.

When in doubt, call `MarkDirty(handle)` after changing texture-bound parameters.

---

## PbrMasterMaterial

`pbr::PbrMasterMaterial` is a convenience layer on top of `MaterialRuntimeBridge`. It manages a permutation cache internally: each feature-flag combination maps to a distinct base material handle. `Build()` selects the correct permutation automatically based on which builder methods were called.

### Setup

```cpp
pbr::PbrMasterMaterial::Config cfg{};
cfg.vs              = vs;
cfg.fs              = fs;
cfg.shadow          = shadow;
cfg.vertexLayout    = layout;
cfg.cullMode        = MaterialCullMode::Back;
cfg.castShadows     = true;
cfg.receiveShadows  = true;

auto master = pbr::PbrMasterMaterial::Create(materialSystem, cfg);
```

### Creating instances via fluent builder

```cpp
MaterialHandle mat = master.CreateInstance("MyCube")
    .BaseColor(gpuAlbedo)       // enables BaseColorMap flag automatically
    .Normal(gpuNormal, 1.5f)    // enables NormalMap flag + sets normalStrength
    .Roughness(0.5f)
    .Metallic(0.2f)
    .IBL(true)
    .DoubleSided(false)
    .Build();
```

The fluent builder sets the appropriate `MaterialFeatureFlags` for each method called. `Build()` looks up or creates the matching permutation and returns an instance handle ready for use.

---

## Constant buffer data

For parameter-driven materials, the system packs parameter values into constant buffer layout data.

```cpp
const std::vector<uint8_t>& cbData   = materials.GetCBData(handle);
const CbLayout&             cbLayout = materials.GetCBLayout(handle);

uint32_t offset = cbLayout.GetOffset("roughnessFactor"); // UINT32_MAX if not found
```

Packing follows the engine's constant-buffer layout rules, aligned to the runtime shader binding model used by the active backend.

---

## Pipeline key and caching

The pipeline state derived from a material is converted into a cache key and looked up in the pipeline cache.

```cpp
PipelineKey key = MaterialRuntimeBridge::BuildPipelineKey(materials, handle);
```

Two materials with identical pipeline-relevant state share the same pipeline key and therefore the same underlying PSO or backend pipeline object, regardless of differing parameter values.

Pipeline-relevant state generally includes:

- shaders
- vertex layout
- render target formats
- depth format
- rasterizer state
- blend state
- depth/stencil state
- pass classification where relevant

Parameter data and material instance values do not by themselves create a separate pipeline unless they imply a different pipeline state.

---

## Sort key

Draw calls are sorted before submission to reduce state changes and preserve correct rendering order.

```cpp
SortKey key = SortKey::ForOpaque    (domain, layer, pipelineHash, linearDepth);
SortKey key = SortKey::ForTransparent(domain, layer, linearDepth);
SortKey key = SortKey::ForUI        (layer, drawOrder);
```

Typical behavior:

- opaque: grouped by pipeline and sorted front-to-back
- transparent: sorted back-to-front
- UI: sorted by explicit draw order or layer

---

## Render policy

`MaterialDesc::renderPolicy` separates engine-level rendering policy from low-level rasterizer, blend, and depth state.

Typical fields:

- `cull.mode` — `MaterialCullMode::Back`, `Front`, or `None`
- `castShadows` — whether the material contributes to the shadow pass
- `receiveShadows` — whether the material receives shadowing

---

## Current practical guidance

Use `PbrMasterMaterial` when:

- authoring standard PBR meshes with metallic/roughness workflow
- wanting automatic permutation management based on which texture slots are bound
- preferring a fluent, asset-oriented creation API

Use `MaterialRuntimeBridge::RegisterMaterial` directly when:

- the shader is custom and does not follow the PBR conventions
- the material targets a non-mesh domain (postprocess, UI, shadow-only, debug)
- fine-grained control over feature flags, texture semantics, and runtime desc is needed

Use explicit `materials.SetFloat` / `SetVec4` / `SetTexture` for runtime parameter variation on existing instances.
