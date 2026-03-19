# Engine Architecture

## Overview

This engine is a modular C++ 3D engine built around a clear separation between runtime control, scene data, render preparation and backend execution. The codebase combines an in-house ECS with a renderer that prepares frame data on the CPU and then hands a fully prepared workload to the active graphics backend. In the current state, **DX11 is the main mature backend** and defines the practical target architecture.

The central design idea is straightforward: the engine does not mix scene logic, visibility processing and low-level draw execution into one large path. Instead, it splits the frame into explicit stages. Scene state lives in the ECS, the renderer converts that state into per-frame render data, a prepared pass graph defines execution order and dependencies, and the backend performs the actual API-specific GPU work.

That makes the architecture easier to understand and easier to evolve. Scene systems can change without rewriting the backend, render passes can be reorganized without breaking the engine host, and API-specific execution remains isolated behind a small interface.

---

## High-Level Structure

```text
GDXEngine
  -> IGDXRenderer / GDXECSRenderer
      -> ECS Registry + Systems
      -> Frame preparation
      -> Culling / Gather
      -> Prepared frame graph
      -> IGDXRenderBackend
          -> DX11 / OpenGL / DX12 backend path
```

At runtime, `GDXEngine` drives the application loop, `GDXECSRenderer` builds the frame from ECS scene data, and the selected backend executes the prepared work on the graphics API. This keeps the frame flow readable: engine host at the top, renderer logic in the middle, backend execution at the bottom.

---

## Runtime Layer

The runtime layer owns the application lifecycle. It creates the window, initializes the renderer, processes events and advances the main loop.

```cpp
class GDXEngine
{
public:
    bool Initialize();
    void Run();
    bool Step();
    void Shutdown();

private:
    std::unique_ptr<IGDXWindow>   m_window;
    std::unique_ptr<IGDXRenderer> m_renderer;
    GDXEventQueue& m_events;
    GDXFrameTimer  m_frameTimer;
};
```

This class is intentionally kept small. It does not decide how rendering works internally. Its job is to keep the engine alive, feed time and events into the renderer and handle startup and shutdown cleanly. That separation matters because it prevents rendering concerns from leaking into the platform and application host code.

---

## Renderer Abstraction

The renderer interface is frame-oriented and compact. It defines the basic lifecycle that every renderer backend path has to support.

```cpp
class IGDXRenderer
{
public:
    virtual ~IGDXRenderer() = default;

    virtual bool Initialize() = 0;
    virtual void BeginFrame() = 0;
    virtual void Tick(float dt) = 0;
    virtual void EndFrame() = 0;
    virtual void Resize(int w, int h) = 0;
    virtual void Shutdown() = 0;
};
```

The actual implementation is `GDXECSRenderer`. This class is the CPU-side render coordinator. It takes the current ECS state, prepares the active views, performs culling, gathers visible work into render queues, builds the prepared frame graph and finally submits the prepared data to the backend.

In other words, this is where the engine turns a scene into a frame.

---

## Backend Abstraction

The backend layer owns API-facing work such as resource creation, constant updates, render pass execution and presentation.

```cpp
class IGDXRenderBackend
{
public:
    virtual bool Initialize(ResourceStore<GDXTextureResource, TextureTag>& texStore) = 0;
    virtual void BeginFrame(const float clearColor[4]) = 0;
    virtual void Present(bool vsync) = 0;
    virtual void Resize(int w, int h) = 0;

    virtual ShaderHandle CreateShader(... ) = 0;
    virtual TextureHandle CreateTexture(... ) = 0;
    virtual bool UploadMesh(MeshAssetResource& mesh) = 0;
    virtual bool CreateMaterialGpu(MaterialResource& mat) = 0;

    virtual void UpdateLights(Registry& registry, FrameData& frame) = 0;
    virtual void UpdateFrameConstants(const FrameData& frame) = 0;
    virtual void* ExecuteRenderPass(const BackendRenderPassDesc& passDesc,
                                    Registry& registry,
                                    const ICommandList& commandList,
                                    ... ) = 0;
};
```

This is one of the most important architectural boundaries in the engine:

- the **renderer** decides what needs to be rendered
- the **backend** decides how that work is executed on a specific API

That split keeps the higher-level render pipeline independent from DX11-specific state binding and draw submission. In practice, the DX11 backend is the most complete and currently acts as the primary production path.

---

## ECS-Centered Scene Model

The scene is built on top of an in-house ECS. Entities are lightweight IDs, components are stored in typed pools and the registry provides typed access and iteration.

```cpp
class Registry
{
public:
    EntityID CreateEntity();
    void DestroyEntity(EntityID id);

    template<typename T, typename... Args>
    T& Add(EntityID id, Args&&... args);

    template<typename T>
    T* Get(EntityID id);

    template<typename First, typename... Rest, typename Func>
    void View(Func&& func);
};
```

This model is used for transforms, cameras, mesh renderers, lights, materials and render-target camera components. The result is a data-oriented scene representation that fits the renderer well: systems can iterate exactly the components they need, and the renderer can build visibility and draw data without being tied to a rigid scene graph.

From an architectural point of view, the ECS is not a side feature. It is the primary source of truth for the scene state that later becomes render work.

---

## Resource Model

GPU-facing resources are managed through typed handles and centralized stores.

```cpp
template<typename T, typename Tag>
class ResourceStore
{
public:
    using HandleType = Handle<Tag>;

    HandleType Add(T resource);
    T* Get(HandleType h);
    bool Release(HandleType h);
    bool IsValid(HandleType h) const;
};
```

This model is used for meshes, materials, shaders, textures, render targets and related GPU resources. It gives the engine a predictable ownership model:

- resources are created and stored centrally
- systems and renderer code pass around typed handles instead of raw pointers
- stale handle detection is possible through generations
- lookups remain uniform across the renderer and backend

That makes the code safer and easier to reason about, especially in a renderer that has to deal with many interdependent objects over multiple frames.

---

## Frame Pipeline

The renderer builds the frame in explicit stages instead of mixing scene traversal, visibility testing and draw submission together.

### Core pipeline phases

- **Prepare** view data, targets, frame snapshots and pass descriptions
- **Cull** visible objects per view and per pass type
- **Gather** build render queues from visible sets
- **Finalize** build execute-ready inputs
- **Execute** submit prepared passes through the backend

The central per-frame structure reflects this layout directly:

```cpp
struct RendererFramePipelineData
{
    FrameData frameSnapshot{};
    ViewPassExecutionData mainView{};
    std::vector<ViewPassExecutionData> rttViews{};
    PreparedFrameGraph frameGraph{};
};
```

This is important because it shows how the engine thinks about rendering. The frame is treated as prepared data, not as a chain of immediate draw calls. First the renderer determines what the frame should contain, then it executes that prepared result in a controlled order.

---

## View-Oriented Rendering

Each view carries the data needed for both preparation and execution.

```cpp
struct ViewPassExecutionData
{
    PreparedViewData prepared{};
    PreparedExecuteData execute{};
    ViewExecutionStats stats{};
    VisibleSet graphicsVisibleSet{};
    VisibleSet shadowVisibleSet{};
    std::vector<RenderGatherSystem::GatherChunkResult> graphicsGatherChunks{};
    std::vector<RenderGatherSystem::GatherChunkResult> shadowGatherChunks{};
    RenderQueue opaqueQueue{};
    RenderQueue transparentQueue{};
    RenderQueue shadowQueue{};
};
```

This gives the pipeline a clear view-oriented structure. Instead of producing one large global queue blob, the renderer handles the main view and each render-target view as separate processing units. Each one can have its own prepared state, visibility results and render queues.

That is especially useful for offscreen rendering, shadow passes and future view extensions. It keeps the pipeline modular and prevents special-case logic from collapsing into one monolithic render path.

---

## Main View and Render-Target Views

The split between the main camera and render-target views is explicit in the task setup:

```cpp
m_systemScheduler.AddTask({ "Prepare Main View",
    SR_FRAME | SR_STATS,
    SR_MAIN_VIEW,
    [this]() { PrepareMainViewData(m_renderPipeline.frameSnapshot, m_renderPipeline.mainView); } });

m_systemScheduler.AddTask({ "Prepare RTT Views",
    SR_FRAME | SR_STATS,
    SR_RTT_VIEWS,
    [this]() { PrepareRenderTargetViewData(m_renderPipeline.frameSnapshot, m_renderPipeline.rttViews); } });
```

This separation continues through culling, gather and execution. That matters because the main presentation path, offscreen rendering and shadow-related views do not share identical requirements. Treating them as independent units keeps the render flow easier to understand and makes it simpler to add more view types later.

---

## Prepared Frame Graph

The engine uses a prepared pass graph with explicit logical resource references and a validated execution order.

```cpp
enum class PreparedFrameGraphNodeKind : uint8_t
{
    RenderTargetShadow = 0,
    RenderTargetGraphics = 1,
    MainShadow = 2,
    MainGraphics = 3,
    MainPresentation = 4
};
```

```cpp
struct PreparedFrameGraphNode
{
    PreparedFrameGraphNodeKind kind = PreparedFrameGraphNodeKind::MainGraphics;
    ViewPassExecutionData* view = nullptr;
    const PreparedExecuteData* executeInput = nullptr;
    ViewExecutionStats*        statsOutput  = nullptr;
    uint32_t viewIndex = 0u;
    bool enabled = false;
    PreparedFrameGraphResourceRef readResource{};
    PreparedFrameGraphResourceRef writeResource{};
    std::vector<uint32_t> dependencies{};
};
```

Logical resources are described separately from the pass nodes:

```cpp
enum class PreparedFrameGraphResourceKind : uint8_t
{
    None = 0,
    BackbufferColor = 1,
    MainSceneColor = 2,
    ShadowMap = 3,
    RenderTargetColor = 4
};
```

This graph describes pass order, logical read/write relationships and dependencies between nodes. It is not a fully generic modern DX12/Vulkan-style render graph. Instead, it is a prepared pass scheduler tailored to the current renderer. For the existing DX11-oriented architecture, that is the right level of abstraction: explicit enough to make dependencies visible and validation possible, but still simple enough to stay practical.

---

## Scheduler and CPU Parallelism

The engine uses two layers of CPU work organization.

### 1. SystemScheduler

A lightweight scheduler groups tasks based on declared read/write masks.

```cpp
struct TaskDesc
{
    std::string name;
    uint64_t readMask = 0ull;
    uint64_t writeMask = 0ull;
    SystemFn fn;
};
```

This is used to describe the high-level frame build in a controlled way. Independent tasks can be grouped conservatively, while conflicting tasks remain ordered. The scheduler therefore acts as the outer orchestration layer of the frame.

### 2. JobSystem

For chunked CPU-side work, the engine uses a worker-thread based `ParallelFor`.

```cpp
void ParallelFor(size_t itemCount,
                 const std::function<void(size_t begin, size_t end)>& fn,
                 size_t minBatchSize = 64u);
```

This is well suited for culling, gather chunk processing and other data-parallel loops. It allows local parallel speedup without turning the full renderer into a deeply nested job graph.

The frame build in `EndFrame()` shows how both layers work together:

```cpp
m_systemScheduler.AddTask({ "Cull Main Graphics",
    SR_MAIN_VIEW | SR_TRANSFORM,
    SR_MAIN_VIEW,
    [this]() { CullPreparedMainViewGraphics(m_renderPipeline.mainView); } });

m_systemScheduler.AddTask({ "Gather Main Graphics",
    SR_MAIN_VIEW | SR_TRANSFORM,
    SR_MAIN_VIEW,
    [this, &resolveShader]() { GatherPreparedMainViewGraphics(resolveShader, m_renderPipeline.mainView); } });

m_systemScheduler.AddTask({ "Build Frame Graph",
    SR_RENDER_QUEUES | SR_BACKEND,
    SR_RENDER_QUEUES,
    [this]() { BuildPreparedFrameGraph(m_renderPipeline); } });

m_systemScheduler.AddTask({ "Execute Prepared Frame",
    SR_RENDER_QUEUES | SR_MAIN_VIEW | SR_RTT_VIEWS | SR_TRANSFORM | SR_BACKEND,
    SR_BACKEND | SR_STATS,
    [this]() { ExecutePreparedFrame(m_renderPipeline); } });
```

This design keeps parallelization under control. The frame has a stable top-level structure, while heavy local loops can still be distributed across worker threads where it makes sense.

---

## Why the Architecture Works

The pieces fit together because each layer has a clear job.

- the **runtime layer** owns the application lifecycle
- the **ECS** stores the scene state
- the **renderer** transforms that scene state into frame-local render data
- the **view pipeline** organizes work per camera or render target
- the **prepared frame graph** defines execution order and logical dependencies
- the **backend** performs the actual API-specific rendering

This gives the engine a stable shape. New passes, new resource types or new views can be added inside the same structure without tearing apart the runtime or hardwiring more API-specific logic into the high-level renderer.

---

## Summary

The architecture can be summarized as follows:

> **A modular ECS-based C++ 3D engine with a view-oriented render pipeline, a prepared pass graph, explicit resource ownership and a backend abstraction centered around a mature DX11 execution path.**

In practice, this means the engine is built around explicit frame preparation instead of immediate rendering, scene data is cleanly separated from backend execution, and rendering work is organized into understandable phases. That makes the codebase suitable for further renderer cleanup, controlled CPU-side optimization and later backend expansion without losing the current DX11-focused stability.
