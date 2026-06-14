# Asset Pipeline

`engine::assets::AssetPipeline`

Loads mesh, texture, shader, and material assets from disk, uploads them to the GPU, and tracks modification timestamps for hot-reload. The pipeline works with an `AssetRegistry` that stores the CPU-side data and a `IDevice` that receives GPU uploads.

---

## Setup

```cpp
assets::AssetRegistry registry;
assets::AssetPipeline pipeline(registry, device, &fs);

pipeline.SetAssetRoot("assets/");
```

The filesystem argument is optional. When omitted, an internal `StdFilesystem` is used.

---

## Loading assets

```cpp
MeshHandle     mesh    = pipeline.LoadMesh    ("meshes/cube.obj");
TextureHandle  texture = pipeline.LoadTexture ("textures/rock.png");
ShaderHandle   vs      = pipeline.LoadShader  ("shaders/unlit.hlslvs");
MaterialHandle mat     = pipeline.LoadMaterial("materials/unlit.json");
```

Handles are valid immediately after the call. GPU upload may be deferred — call `UploadPendingGpuAssets()` before rendering.

---

## Uploading to the GPU

```cpp
pipeline.UploadPendingGpuAssets();
```

Transfers all loaded CPU assets that have not yet been uploaded to the GPU. Call once per frame before the render pass, or after loading a batch of assets.

GPU handles are separate from CPU handles:

```cpp
TextureHandle gpuTex = pipeline.GetGpuTexture(texture);
ShaderHandle  gpuVS  = pipeline.GetGpuShader(vs);
```

Pass GPU handles to `IDevice` and material bindings. Pass CPU handles to the registry for asset data queries.

---

## Shader cache

```cpp
pipeline.BuildShaderCache(vs, assets::ShaderTargetProfile::DirectX11_SM5);
pipeline.BuildPendingShaderCaches();
```

Pre-compiles shader variants to bytecode. `BuildPendingShaderCaches` processes all queued shaders in one pass — useful during a loading screen.

---

## Loading a scene

```cpp
Scene scene(world);
pipeline.LoadScene("scenes/level01.json", scene);
```

Deserializes entity and component data from JSON into the world. Mesh and material references are resolved through the registry.

---

## Hot-reload

```cpp
// Call once per frame (or on a timer)
pipeline.PollHotReload();
```

Compares `lastModifiedTimestamp` values against the values stored in each asset. Assets whose files have changed on disk are reloaded and their GPU resources are re-uploaded on the next `UploadPendingGpuAssets` call.

---

## Asset states

| State | Meaning |
|---|---|
| `Unloaded` | Not yet requested |
| `Loading` | In progress |
| `Loaded` | CPU data available |
| `Failed` | Load error |
| `Evicted` | Previously loaded, GPU resource outdated (hot-reload pending) |
