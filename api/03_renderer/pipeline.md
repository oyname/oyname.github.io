# Pipeline

`engine::renderer::PipelineDesc` · `engine::renderer::PipelineCache`

A pipeline encodes the complete GPU state for a draw or dispatch call: shaders, vertex layout, rasterizer, blend, depth-stencil, and target formats. Pipelines are created once and reused — `PipelineCache` deduplicates them by hash.

---

## PipelineDesc

```cpp
PipelineDesc desc{};
desc.shaderStages = {
    { vsHandle, ShaderStageMask::Vertex,   "main" },
    { psHandle, ShaderStageMask::Fragment, "main" },
};

desc.vertexLayout.attributes = {
    { VertexSemantic::Position, Format::RGB32_FLOAT, 0, 0  },
    { VertexSemantic::Normal,   Format::RGB32_FLOAT, 0, 12 },
    { VertexSemantic::TexCoord0,Format::RG32_FLOAT,  0, 24 },
};
desc.vertexLayout.bindings = {
    { 0, 32, VertexInputRate::PerVertex },
};

desc.rasterizer.cullMode  = CullMode::Back;
desc.rasterizer.frontFace = WindingOrder::CCW;

desc.depthStencil.depthEnable = true;
desc.depthStencil.depthWrite  = true;
desc.depthStencil.depthFunc   = DepthFunc::Less;

desc.colorFormat = Format::RGBA16_FLOAT;
desc.depthFormat = Format::D24_UNORM_S8_UINT;
desc.topology    = PrimitiveTopology::TriangleList;

PipelineHandle pipeline = device->CreatePipeline(desc);
```

---

## Compute pipeline

```cpp
PipelineDesc compute{};
compute.pipelineClass = PipelineClass::Compute;
compute.shaderStages  = {
    { csHandle, ShaderStageMask::Compute, "main" },
};

PipelineHandle csPipeline = device->CreatePipeline(compute);
```

---

## Rasterizer state

| Field | Default | Notes |
|---|---|---|
| `fillMode` | `Solid` | `Wireframe` for debug |
| `cullMode` | `Back` | `None` disables culling |
| `frontFace` | `CCW` | Use `CW` for LookAtRH cameras |
| `depthBias` | `0.f` | Offset for shadow maps |
| `slopeScaledBias` | `0.f` | PCF shadow quality |

---

## Blend state

Transparent materials require blend enabled:

```cpp
BlendState blend{};
blend.blendEnable   = true;
blend.srcBlend      = BlendFactor::SrcAlpha;
blend.dstBlend      = BlendFactor::InvSrcAlpha;
blend.blendOp       = BlendOp::Add;
blend.srcBlendAlpha = BlendFactor::One;
blend.dstBlendAlpha = BlendFactor::Zero;
blend.blendOpAlpha  = BlendOp::Add;

desc.blendStates[0] = blend;
```

---

## PipelineCache

`engine::renderer::PipelineCache` — `renderer/PipelineCache.hpp`

Stores and retrieves pipelines by `PipelineKey`. Materials use this cache internally — direct access is only needed when building pipelines outside the material system.

```cpp
PipelineCache cache;
cache.Initialize(device);

PipelineHandle p = cache.GetOrCreate(pipelineDesc, passTag);
cache.Clear();
```

Pipelines are keyed by a hash of the full `PipelineDesc`. Two descriptions that produce identical state return the same handle.

---

## Destroying a pipeline

```cpp
device->DestroyPipeline(pipeline);
```

Do not destroy pipelines that may still be in flight on the GPU. Wait for the frame fence before destroying.
