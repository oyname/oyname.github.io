# Material

`engine::renderer::MaterialSystem` · `engine::renderer::MaterialDesc`

Materials describe how a mesh is shaded: which shaders to use, which render pass it belongs to, pipeline state, and shader parameters. `MaterialSystem` manages descriptions and instances. Instances share a pipeline but can carry their own parameter values.

---

## Registering a material

```cpp
MaterialDesc desc{};
desc.name          = "UnlitWhite";
desc.passTag       = RenderPassTag::Opaque;
desc.vertexShader  = vsHandle;
desc.fragmentShader = fsHandle;
desc.colorFormat   = Format::RGBA16_FLOAT;  // HDR default
desc.depthFormat   = Format::D24_UNORM_S8_UINT;

desc.params.push_back({ "baseColor", MaterialParam::Type::Vec4 });
desc.params.back().value.f[0] = 1.f;
desc.params.back().value.f[1] = 1.f;
desc.params.back().value.f[2] = 1.f;
desc.params.back().value.f[3] = 1.f;

MaterialHandle mat = materials.RegisterMaterial(std::move(desc));
```

---

## Creating instances

```cpp
MaterialHandle redInstance = materials.CreateInstance(mat, "UnlitRed");
materials.SetVec4(redInstance, "baseColor", { 1.f, 0.f, 0.f, 1.f });
```

Instances share the underlying `PipelineKey` — no additional PSO is created unless pipeline state differs.

---

## Setting parameters

```cpp
materials.SetFloat  (handle, "roughness", 0.4f);
materials.SetVec4   (handle, "baseColor", { 0.8f, 0.6f, 0.2f, 1.f });
materials.SetTexture(handle, "albedoMap", texHandle);
materials.MarkDirty (handle);
```

`SetFloat` and `SetVec4` mark the material dirty automatically. `SetTexture` requires an explicit `MarkDirty` call.

---

## Constant buffer data

The system packs parameters into a constant buffer layout following HLSL cbuffer rules:

```cpp
const std::vector<uint8_t>& cbData   = materials.GetCBData(handle);
const CbLayout&             cbLayout = materials.GetCBLayout(handle);

uint32_t offset = cbLayout.GetOffset("roughness");  // UINT32_MAX if not found
```

---

## Render pass tags

| Tag | Usage |
|---|---|
| `RenderPassTag::Opaque` | Default forward pass, front-to-back sorted |
| `RenderPassTag::AlphaCutout` | Masked geometry, depth-tested |
| `RenderPassTag::Transparent` | Back-to-front sorted, blending enabled |
| `RenderPassTag::Shadow` | Shadow map pass |
| `RenderPassTag::UI` | Overlay pass, no depth test |
| `RenderPassTag::Postprocess` | Full-screen passes |

---

## Pipeline key

The `PipelineKey` is derived from the `MaterialDesc` and cached. It encodes all pipeline state (shaders, rasterizer, blend, depth, formats) into a 64-bit hash used to look up or create the PSO.

```cpp
PipelineKey key = materials.BuildPipelineKey(handle);
```

Two materials with identical pipeline state share the same `PipelineKey` and therefore the same underlying PSO — regardless of their parameter values.

---

## Sort key

Draw calls are sorted by `SortKey` before submission. Opaque draws sort front-to-back and group by pipeline hash to minimize state changes.

```cpp
SortKey key = SortKey::ForOpaque(passTag, layer, pipelineHash, linearDepth);
SortKey key = SortKey::ForTransparent(passTag, layer, linearDepth);
SortKey key = SortKey::ForUI(layer, drawOrder);
```
