# IBL / Environment

`engine::renderer::EnvironmentDesc` · `engine::renderer::EnvironmentSystem` · `engine::renderer::IBLBaker`

The IBL (Image-Based Lighting) system provides environment-based diffuse irradiance and specular prefiltered radiance for PBR shading. It converts HDR environment textures into baked cubemaps, serializes them as `.iblcache` files for offline reuse, and feeds the resulting GPU textures into the shader binding model.

---

## EnvironmentDesc

`EnvironmentDesc` is the descriptor passed to `RenderSystem::CreateEnvironment`.

```cpp
struct EnvironmentDesc {
    EnvironmentMode mode;           // Texture, Color, None
    TextureHandle   sourceTexture;  // HDR source texture (e.g. loaded from .hdr)
    float           intensity = 1.0f;
    bool            enableIBL = true;
};
```

### EnvironmentMode

| Value | Meaning |
|---|---|
| `EnvironmentMode::Texture` | Use a HDR texture as the environment source |
| `EnvironmentMode::Color` | Use a constant color as the environment |
| `EnvironmentMode::None` | No environment contribution |

---

## Setting up an environment

```cpp
renderer::EnvironmentDesc env{};
env.mode          = renderer::EnvironmentMode::Texture;
env.sourceTexture = assetPipeline.LoadTexture("autumn_field_2k.hdr");
env.intensity     = 0.5f;
env.enableIBL     = true;

auto handle = renderSystem.CreateEnvironment(env);
renderSystem.SetActiveEnvironment(handle);
```

After `SetActiveEnvironment`, the render system makes the baked IBL data available to the shader runtime for the current frame. Materials that have `MaterialFeatureFlags::IblMap` set will receive irradiance and prefiltered radiance automatically.

---

## EnvironmentSystem

`EnvironmentSystem` manages the lifecycle of IBL data at runtime.

Responsibilities:

- invoking `IBLBaker` to produce irradiance and prefiltered cubemaps from a source HDR texture
- managing the `.iblcache` files through `IBLCacheSerializer`
- loading cached bakes via `IBLResourceLoader` to avoid repeated GPU baking
- uploading GPU textures to `ShaderRuntime` so they are available to shaders

The system is transparent to application code. `CreateEnvironment` and `SetActiveEnvironment` on `RenderSystem` are the only calls most users need.

---

## IBLBaker

`IBLBaker` performs the actual baking work:

- **Irradiance cubemap** — low-frequency diffuse integral over the hemisphere, stored as a cubemap
- **Prefiltered environment cubemap** — specular contribution at multiple roughness levels, stored as a mip-chain cubemap

Both outputs use the `RGBA16F` format to balance precision and memory usage.

Baked results are written to `.iblcache` files by `IBLCacheSerializer`. On subsequent runs, `IBLResourceLoader` reads the cache and bypasses the baking step entirely.

---

## IBLCacheSerializer

`IBLCacheSerializer` handles serialization and deserialization of `.iblcache` files.

- serializes the baked irradiance and prefiltered cubemaps to disk after a bake
- deserializes existing cache files so the GPU textures can be uploaded without rebaking
- the cache file stores RGBA16F data for both cubemaps

The `.iblcache` format is an engine-internal binary format. Do not modify cache files manually.

---

## IBLResourceLoader

`IBLResourceLoader` bridges the cache and the shader runtime:

1. loads the `.iblcache` file via `IBLCacheSerializer`
2. uploads irradiance, prefiltered, and BRDF LUT textures to the GPU
3. binds them to the fixed texture slots used by the shader runtime

---

## Shader texture slots

IBL data occupies fixed texture slots in the engine's shader binding model:

| Slot constant | Index | Content |
|---|---|---|
| `TexSlots::IBLIrradiance` | `5` | Irradiance cubemap |
| `TexSlots::IBLPrefiltered` | `6` | Prefiltered environment cubemap |
| `TexSlots::BRDFLUT` | `7` | Split-sum BRDF look-up table |

These slots are reserved and must not be used by material parameter bindings.

---

## IBL flag in materials

The `MaterialFeatureFlags::IblMap` flag enables IBL contribution in a material's shader permutation.

When an active environment is set, `ShaderRuntime::PrepareMaterial` sets this flag automatically on materials that support it. Materials built with `pbr::PbrMasterMaterial` enable IBL via the fluent builder:

```cpp
MaterialHandle mat = master.CreateInstance("MyMesh")
    .BaseColor(gpuAlbedo)
    .Roughness(0.4f)
    .Metallic(0.0f)
    .IBL(true)       // enables MaterialFeatureFlags::IblMap
    .Build();
```

If no active environment is present, the IBL contribution evaluates to zero regardless of the flag.

---

## Typical workflow

1. Load a `.hdr` file through the asset pipeline to obtain a `TextureHandle`.
2. Fill an `EnvironmentDesc` with `mode = Texture`, the texture handle, and a desired `intensity`.
3. Call `renderSystem.CreateEnvironment(env)` to obtain an `EnvironmentHandle`.
4. Call `renderSystem.SetActiveEnvironment(handle)` once per scene or when the environment changes.
5. Use `MaterialFeatureFlags::IblMap` or `.IBL(true)` on PBR materials to opt in.

The engine handles baking, caching, and shader binding transparently.

---

## Notes on performance

- The initial bake is a GPU-side compute operation and is only performed once per unique source texture.
- Baked results are cached to disk as `.iblcache`. Subsequent application runs load from cache and skip the bake entirely.
- RGBA16F format keeps GPU memory usage low compared to RGBA32F while maintaining adequate HDR range for typical environment maps.
- Changing the active environment mid-frame is supported but will stall the baking pipeline if the new environment has no cache entry yet.
