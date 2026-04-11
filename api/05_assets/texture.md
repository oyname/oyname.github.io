# Texture

`engine::assets::TextureAsset` · `engine::TextureHandle`

Texture assets hold CPU-side pixel data and metadata. The GPU representation is a `TextureHandle` that maps to a backend resource.

---

## Loading a texture

```cpp
TextureHandle handle = pipeline.LoadTexture("textures/rock_albedo.png");
pipeline.UploadPendingGpuAssets();

TextureHandle gpuHandle = pipeline.GetGpuTexture(handle);
```

Pass `gpuHandle` to material bindings and shader resource slots. Use the CPU `handle` to query the registry for pixel data or asset state.

---

## Accessing CPU data

```cpp
const assets::TextureAsset* asset = registry.GetTexture(handle);
if (!asset) return;

asset->width       // pixels
asset->height
asset->depth       // 1 for 2D textures
asset->mipLevels
asset->arraySize
asset->format      // TextureFormat enum
asset->isCubemap
asset->sRGB
asset->pixelData   // raw bytes
```

---

## Texture formats

| Format | Use |
|---|---|
| `RGBA8_UNORM` | General-purpose color |
| `RGBA8_SRGB` | Albedo / diffuse (gamma-correct) |
| `R8_UNORM` | Single-channel mask or roughness |
| `RG8_UNORM` | Two-channel (e.g. packed normals) |
| `BC1` | DXT1 — opaque color compression |
| `BC3` | DXT5 — color + alpha |
| `BC4` | Single-channel compressed (roughness, AO) |
| `BC5` | Two-channel compressed (normal maps XY) |
| `BC7` | High-quality color + alpha |
| `RGBA16F` | HDR render targets, light maps |
| `R11G11B10F` | HDR color without alpha |
| `DEPTH24_STENCIL8` | Depth-stencil buffer |
| `DEPTH32F` | High-precision depth |

---

## Binding a texture to a material

```cpp
materials.SetTexture(matHandle, "albedoMap", gpuHandle);
materials.MarkDirty(matHandle);
```

---

## Asset state

```cpp
asset->state               // AssetState::Loaded, ::Failed, ...
asset->path                // file path
asset->gpuStatus.uploaded  // true after GPU upload
asset->gpuStatus.dirty     // true when hot-reload is pending
```

---

## Cubemaps

```cpp
const assets::TextureAsset* asset = registry.GetTexture(handle);
bool isCubemap = asset->isCubemap;  // true for cube textures
// arraySize == 6 for a standard cubemap
```

Cubemaps loaded through `AssetPipeline::LoadTexture` are detected automatically from the source format. Use them for skyboxes and IBL environment maps.
