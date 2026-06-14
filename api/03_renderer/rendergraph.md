# RenderGraph

`engine::rendergraph::RenderGraph`

A frame graph that manages pass ordering, resource lifetimes, and barrier placement. Each frame is described by declaring passes and the resources they read or write. The graph compiles a dependency-sorted execution plan and issues barriers automatically.

---

## Basic setup

```cpp
using namespace engine::rendergraph;

RenderGraph rg;
rg.Reset();

// Import the swapchain backbuffer as an output resource
RGResourceID backbuffer = rg.ImportBackbuffer(rtHandle, texHandle, width, height);
```

---

## Declaring passes

```cpp
rg.AddPass("ForwardOpaque")
    .WriteRenderTarget(backbuffer)
    .WriteDepthStencil(depthResource)
    .Execute([&](const RGExecContext& ctx) {
        auto* cmd = ctx.cmd;
        cmd->SetPipeline(pipeline);
        cmd->Draw(vertexCount);
    });
```

Passes declare their resource access upfront. The graph uses these declarations to determine ordering and to plan transitions — the execute lambda does not need to insert barriers manually.

---

## Resource types

| Method | Kind |
|---|---|
| `ImportBackbuffer` | Swapchain render target |
| `ImportRenderTarget` | Existing RT + texture pair |
| `ImportTexture` | Read-only imported texture |
| `CreateTransientRenderTarget` | Allocated and freed by the graph |

```cpp
RGResourceID hdrColor = rg.CreateTransientRenderTarget(
    "HDRColor", width, height, Format::RGBA16_FLOAT);

RGResourceID shadowMap = rg.CreateTransientRenderTarget(
    "ShadowMap", 2048, 2048, Format::D32_FLOAT,
    RGResourceKind::ShadowMap);
```

---

## Access declarations

| Builder method | Meaning |
|---|---|
| `WriteRenderTarget(id)` | Output color attachment |
| `WriteDepthStencil(id)` | Depth write |
| `ReadDepthStencil(id)` | Depth read (e.g. shadow sampling) |
| `ReadTexture(id)` | Shader resource view |
| `WriteStorageBuffer(id)` | UAV / compute write |
| `Present(id)` | Transition to present |
| `ReadHistoryBuffer(id)` | Previous frame's value |
| `WriteHistoryBuffer(id)` | Current frame's value |

---

## History resources (TAA, SSR, accumulation)

```cpp
RenderGraph::HistoryPair gtao = rg.ImportHistoryPair(
    curRT, curTex, prevRT, prevTex,
    "GTAO", width, height, Format::R8_UNORM);

rg.AddPass("GTAO")
    .WriteHistoryBuffer(gtao.current)
    .ReadHistoryBuffer(gtao.prev)
    .Execute([&](const RGExecContext& ctx) { ... });

// After Execute(), swap for the next frame
rg.SwapHistoryResources(gtao.current, gtao.prev);
```

---

## Compile and execute

```cpp
CompiledFrame frame;

if (rg.Compile(frame)) {
    RGExecContext ctx{};
    ctx.device = device;
    ctx.cmd    = commandList;

    rg.Execute(frame, ctx);
}
```

`Compile` returns `false` if the graph has validation errors (cycles, stale reads, missing producers). `CompiledFrame` can be reused across frames when the topology has not changed — the graph detects this via a topology key.

---

## Resolving transient resources in the execute lambda

```cpp
.Execute([&](const RGExecContext& ctx) {
    RenderTargetHandle rt  = ctx.GetRenderTarget(hdrColor);
    TextureHandle      tex = ctx.GetTexture(shadowMap);
    BufferHandle       buf = ctx.GetBuffer(someBuffer);
})
```

---

## Barrier optimization

```cpp
BarrierStats stats = RenderGraph::OptimizeBarriers(frame);
```

Merges redundant transitions within the compiled frame. Called automatically during `Compile` — exposed for testing and diagnostics.
