# Mesh

`engine::assets::MeshAsset` · `engine::MeshHandle`

Mesh assets hold CPU-side vertex and index data organized into submeshes. Each submesh maps to one material slot. The GPU representation is a separate `BufferHandle` pair managed by the renderer.

---

## Loading a mesh

```cpp
MeshHandle handle = pipeline.LoadMesh("meshes/cube.obj");
```

After loading, the CPU data is in the registry. Upload it before rendering:

```cpp
pipeline.UploadPendingGpuAssets();
```

---

## Accessing CPU data

```cpp
const assets::MeshAsset* asset = registry.GetMesh(handle);
if (!asset) return;

for (const auto& sub : asset->submeshes) {
    // sub.positions  — 3 floats per vertex: x, y, z
    // sub.normals    — 3 floats per vertex
    // sub.tangents   — 3 floats per vertex
    // sub.uvs        — 2 floats per vertex: u, v
    // sub.indices    — uint32_t index list
    // sub.materialIndex
}
```

---

## Bounds

```cpp
math::Vec3 bmin, bmax;
asset->ComputeBounds(bmin, bmax);

math::Vec3 center  = (bmin + bmax) * 0.5f;
math::Vec3 extents = (bmax - bmin) * 0.5f;
```

`ComputeBounds` scans all vertex positions across all submeshes. The result can be written directly into a `BoundsComponent`:

```cpp
auto& b = world.Add<BoundsComponent>(entity);
b.centerLocal  = center;
b.extentsLocal = extents;
```

---

## Attaching a mesh to an entity

```cpp
world.Add<MeshComponent>(entity, handle);
world.Add<MaterialComponent>(entity, matHandle);
world.Add<BoundsComponent>(entity);
```

The renderer reads `MeshComponent.mesh` and looks up the GPU buffers from the asset registry during draw list construction.

---

## Supported formats

`AssetPipeline::LoadMesh` infers the format from the file extension. OBJ files are supported by default. The submesh data layout is always:

- positions: packed `float[3]` per vertex
- normals: packed `float[3]` per vertex (may be empty)
- tangents: packed `float[3]` per vertex (may be empty)
- UVs: packed `float[2]` per vertex (may be empty)
- indices: `uint32_t` triangle list

---

## Asset state

```cpp
asset->state        // AssetState::Loaded, ::Failed, etc.
asset->path         // original file path
asset->gpuStatus.uploaded  // true once GPU buffers are ready
asset->gpuStatus.dirty     // true after hot-reload, before re-upload
```
