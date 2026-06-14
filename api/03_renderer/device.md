# Device

`engine::renderer::IDevice` · `engine::renderer::ISwapchain` · `engine::renderer::IFence` · `engine::renderer::DeviceFactory`

`IDevice` is the engine's central GPU abstraction. It owns backend initialization, resource creation and destruction, upload paths, pipeline creation, command-list creation, synchronization primitives, frame boundaries, and runtime capability reporting. Backends such as DX11, OpenGL, Vulkan, and Null are implemented in separate AddOns and register themselves through `DeviceFactory`.

The interface is intentionally backend-neutral. Core code talks to `IDevice`; backend-specific headers stay in the AddOns.

---

## Responsibilities

A device is responsible for:

- backend initialization and shutdown
- adapter selection
- swapchain creation
- buffer, texture, shader, render-target, sampler, and pipeline creation
- command-list creation per queue
- upload of buffer and texture data
- frame begin/end boundaries
- fence creation and waiting
- reporting backend capabilities and queue/runtime information
- exposing current physical resource-state information to higher-level systems such as RenderGraph

`IDevice` does **not** describe scene data, materials, render passes, or ECS extraction. Those are handled elsewhere in the renderer stack.

---

## Creating a device

Devices are created through `DeviceFactory`. Backends register themselves automatically through static registration in their AddOn modules.

Typical flow:

1. make sure the backend is registered
2. enumerate adapters
3. choose an adapter index
4. create the device
5. initialize it with `IDevice::DeviceDesc`
6. create a swapchain for the window

Example flow:

- check registration with `DeviceFactory::IsRegistered(...)`
- call `DeviceFactory::EnumerateAdapters(...)`
- select an adapter with `DeviceFactory::FindBestAdapter(...)`
- create the backend with `DeviceFactory::Create(...)`
- call `Initialize(desc)`

Available backend types in the current API:

- `DeviceFactory::BackendType::Null`
- `DeviceFactory::BackendType::DirectX11`
- `DeviceFactory::BackendType::DirectX12`
- `DeviceFactory::BackendType::OpenGL`
- `DeviceFactory::BackendType::Vulkan`

Notes:

- `Create(...)` can fall back to the Null backend if the requested backend is not available and Null is registered.
- `EnumerateAdapters(...)` returns an empty vector if the backend is not registered or does not expose enumeration.

---

## Device initialization parameters

`IDevice::DeviceDesc` controls how the backend is initialized.

Fields:

| Field | Type | Meaning |
|---|---|---|
| `enableDebugLayer` | `bool` | Enables backend-specific debug output or validation when available. Useful during development. |
| `enableGpuValidation` | `bool` | Requests stronger GPU-side validation when the backend supports it. More expensive than normal debug validation. |
| `adapterIndex` | `uint32_t` | Selects the adapter returned by backend enumeration. Usually set from `FindBestAdapter(...)`. |
| `appName` | `std::string` | Application name forwarded to the backend for diagnostics, metadata, logs, and tooling integration. |

Important clarifications:

- `appName` is **not** the window title. The window title belongs to the platform window description.
- `adapterIndex` matters on systems with multiple GPUs.
- `enableDebugLayer` and `enableGpuValidation` are primarily development settings.

---

## Adapter enumeration

`DeviceFactory::EnumerateAdapters(...)` returns a vector of `AdapterInfo` structures.

`AdapterInfo` fields:

| Field | Type | Meaning |
|---|---|---|
| `index` | `uint32_t` | Value to pass into `DeviceDesc::adapterIndex`. |
| `name` | `std::string` | Human-readable UTF-8 adapter name. |
| `dedicatedVRAM` | `size_t` | Dedicated VRAM in bytes when known. Can be `0` if unknown. |
| `isDiscrete` | `bool` | `true` for a discrete GPU, `false` for an integrated GPU. |
| `featureLevel` | `int` | Backend-specific normalized capability score. DX11 uses values like `110` or `121`. OpenGL uses `major*10+minor`, for example `46` for GL 4.6. `0` means unknown. |

`DeviceFactory::FindBestAdapter(...)` currently chooses:

- highest `featureLevel` first
- if tied, discrete GPU wins over integrated GPU

It returns `0` when the adapter list is empty.

---

## Swapchain

Use `IDevice::CreateSwapchain(...)` to create the presentation chain for a native window.

`IDevice::SwapchainDesc` fields:

| Field | Type | Meaning |
|---|---|---|
| `nativeWindowHandle` | `void*` | Native platform window handle. Required for presentation. |
| `width` | `uint32_t` | Backbuffer width. |
| `height` | `uint32_t` | Backbuffer height. |
| `bufferCount` | `uint32_t` | Number of backbuffers. |
| `format` | `Format` | Swapchain color format. Defaults to `Format::BGRA8_UNORM_SRGB`. |
| `vsync` | `bool` | Presentation synchronization preference. |
| `debugName` | `std::string` | Optional name for diagnostics. |
| `openglMajor` | `int` | Requested OpenGL major version. Ignored by non-OpenGL backends. |
| `openglMinor` | `int` | Requested OpenGL minor version. Ignored by non-OpenGL backends. |
| `openglDebugContext` | `bool` | Requests an OpenGL debug context. Ignored by non-OpenGL backends. |

This is easy to miss: OpenGL-specific context versioning lives in the **swapchain description**, not in `DeviceDesc`.

### `ISwapchain` operations

| Method | Meaning |
|---|---|
| `AcquireForFrame()` | Acquire the next backbuffer for rendering. |
| `Present(bool vsync)` | Present the current backbuffer. |
| `Resize(width, height)` | Resize the swapchain resources. |
| `GetCurrentBackbufferIndex()` | Returns the active backbuffer index. |
| `GetBackbufferTexture(index)` | Returns the backbuffer texture handle. |
| `GetBackbufferRenderTarget(index)` | Returns the backbuffer render-target handle. |
| `GetWidth()` / `GetHeight()` | Current swapchain extent. |
| `CanRenderFrame()` | Whether the swapchain is currently renderable. |
| `NeedsRecreate()` | Whether the swapchain needs recreation. |
| `QueryFrameStatus()` | Returns structured frame-status information. |
| `GetRuntimeDesc()` | Returns backend/runtime details about the swapchain implementation. |

---

## Resource creation

### Buffers

Create with `CreateBuffer(const BufferDesc&)` and destroy with `DestroyBuffer(BufferHandle)`.

Data access paths:

- `MapBuffer(...)` / `UnmapBuffer(...)` for CPU-writable buffer types
- `UploadBufferData(...)` for explicit upload into an existing GPU resource

Use cases include:

- vertex buffers
- index buffers
- constant/uniform buffers
- staging or upload buffers
- compute resources where supported by the backend/runtime

### Textures

Create with `CreateTexture(const TextureDesc&)` and destroy with `DestroyTexture(TextureHandle)`.

Upload with:

- `UploadTextureData(handle, data, byteSize, mipLevel, arraySlice)`

This path is used for regular textures and can also feed semantic material inputs after asset upload/runtime translation.

### Render targets

Create with `CreateRenderTarget(const RenderTargetDesc&)` and destroy with `DestroyRenderTarget(RenderTargetHandle)`.

You can query the attached textures through:

- `GetRenderTargetColorTexture(rt)`
- `GetRenderTargetDepthTexture(rt)`

This matters for post-process and multi-pass rendering, where the render target itself and the textures derived from it are both needed.

### Shaders

Shaders can be created from:

- source via `CreateShaderFromSource(...)`
- precompiled bytecode via `CreateShaderFromBytecode(...)`

Destroy with `DestroyShader(...)`.

The exact bytecode or source path depends on the backend. In current project usage:

- DX11 commonly uses HLSL source and backend compilation
- OpenGL commonly uses GLSL source
- Vulkan requires SPIR-V-capable handling, either loaded or produced through the shader pipeline/runtime

### Pipelines

Create with `CreatePipeline(const PipelineDesc&)` and destroy with `DestroyPipeline(...)`.

Pipelines are backend objects built from pipeline state such as:

- shaders
- blend state
- rasterizer state
- depth/stencil state
- vertex input layout
- render target formats
- primitive topology and related state encoded by `PipelineDesc`

Higher-level systems such as `MaterialSystem` and the pipeline cache typically decide when a pipeline needs to be created.

### Samplers

Create with `CreateSampler(const SamplerDesc&)`.

Samplers are device-level objects in the abstraction. Binding is performed later by command lists and backend descriptor/binding paths.

---

## Command lists and queues

Use `CreateCommandList(QueueType queue)` to create a command list for a specific queue.

Relevant queue types in the current runtime model:

- `QueueType::Graphics`
- `QueueType::Compute`
- `QueueType::Transfer`

Queue support is backend dependent. Do not assume dedicated compute or transfer queues exist everywhere.

Use runtime queries instead:

- `GetQueueCapabilities(queue)`
- `GetPreferredUploadQueue()`
- `GetCommandListRuntime()`
- `GetUploadRuntime()`
- `GetComputeRuntime()`

### `QueueCapabilities`

`GetQueueCapabilities(...)` reports whether a queue:

- is supported
- is dedicated
- can present

This is important because DX11/OpenGL style backends usually behave like graphics-queue-centric implementations, while Vulkan can expose richer queue separation.

---

## Fences

Create synchronization primitives with `CreateFence(initialValue)`.

`IFence` exposes:

| Method | Meaning |
|---|---|
| `Signal(value)` | Signal the fence to a given value. |
| `Wait(value, timeoutNs)` | Wait until the fence reaches the requested value. |
| `GetValue()` | Return the current fence value. |

Fences are used for CPU/GPU and inter-submission synchronization where the backend/runtime supports it.

---

## Upload path

The device exposes explicit upload helpers:

- `UploadBufferData(...)`
- `UploadTextureData(...)`

Upload behavior is described further by `GetUploadRuntime()`.

The current runtime description communicates:

- which queue records uploads
- whether uploads are graphics-queue-only or use a deferred multi-queue path

This is especially relevant because the engine is already shaped around explicit multi-queue and ownership-transfer concepts even though backend maturity differs.

---

## Frame boundaries

Frame lifecycle is explicit:

- `BeginFrame()`
- record and submit command work
- `EndFrame()`
- present through the swapchain

Typical responsibilities of the device at frame boundaries include:

- resetting per-frame counters
- advancing swapchain/backbuffer state
- preparing backend frame resources
- flushing deferred work when necessary

Presentation itself is handled through `ISwapchain::Present(vsync)`.

---

## Resource-state introspection

The device is the authoritative source of the current physical resource states.

This is exposed through:

- `QueryBufferState(...)`
- `QueryTextureState(...)`
- `QueryRenderTargetState(...)`
- `QueryBufferAllocation(...)`
- `QueryTextureAllocation(...)`

Why this matters:

- RenderGraph planning can inspect real backend-known resource states
- state tracking does not need to be duplicated as a second source of truth in higher-level systems
- queue ownership, transitions, and allocation details can be queried from the backend layer

This is a major part of the engine's current explicit-runtime direction.

---

## Runtime description queries

The device can describe its own runtime behavior through structured queries rather than ad-hoc backend checks.

### `GetDescriptorRuntimeLayout()`

Returns the engine descriptor layout description used by the backend. This is relevant for engine-semantic bindings, descriptor slots, and material/runtime binding conventions.

### `GetComputeRuntime()`

Returns a `ComputeRuntimeDesc` describing:

- which queue records compute work
- whether compute is graphics-queue-only or prefers dedicated compute
- synchronization model for compute-to-graphics interaction
- whether compute pipelines and dispatch are supported
- whether dedicated compute queues are enabled
- maturity of the compute path

### `GetCommandListRuntime()`

Returns a `CommandListRuntimeDesc` describing:

- supported queues
- queue dedication and presentation capability
- queue synchronization model
- ownership-transfer preparation/materialization
- graph-to-submission mapping behavior
- multi-queue preparation and materialization state
- compute runtime integration

### `GetUploadRuntime()`

Returns an `UploadRuntimeDesc` describing:

- the preferred recording queue for uploads
- the submission model for uploads

### `GetSwapchainRuntime()`

Returns backend/runtime details specific to presentation and swapchain handling.

These queries are important because the engine is not a pure DX11-style immediate-mode abstraction anymore. It exposes explicit runtime structure and queue behavior in a backend-neutral way.

---

## Diagnostics and feature queries

The device exposes lightweight runtime diagnostics:

| Method | Meaning |
|---|---|
| `GetDrawCallCount()` | Number of draw calls recorded in the last frame. |
| `GetBackendName()` | Backend identifier such as `"DirectX11"`, `"OpenGL"`, `"Vulkan"`, or `"Null"`. |
| `SupportsFeature(feature)` | Backend-specific feature query, for example compute support. |

In the current codebase, backends implement `SupportsFeature(...)` individually. Treat it as a runtime capability query, not as a compile-time guarantee.

---

## Shutdown and destruction

Before shutting the device down, destroy resources that were created from it:

- buffers
- textures
- render targets
- shaders
- pipelines
- backend-managed objects tied to the device lifetime

Then:

- `WaitIdle()` if needed
- `Shutdown()`

This avoids dangling backend state and makes teardown predictable.

---

## Relationship to higher-level systems

`IDevice` is deliberately low-level relative to the rest of the renderer stack.

Common layering in the current engine:

- `DeviceFactory` picks and creates the backend device
- `IDevice` owns backend resource and queue behavior
- `RenderSystem` and `PlatformRenderLoop` coordinate device, swapchain, frame stages, and features
- `MaterialSystem` describes pipeline-relevant shading state and material inputs
- `PipelineCache` avoids duplicate backend PSO creation
- `ShaderRuntime` and related systems bridge engine semantics to backend binding models
- `RenderGraph` plans passes/resources using backend state queries where needed

So the device is the execution and resource layer, not the scene/material authoring layer.

---

## Current design direction

The current interface is no longer just a thin DX11 wrapper. It already exposes a more explicit runtime model influenced by DX12/Vulkan-style concepts:

- queue-aware command-list creation
- upload-runtime description
- compute-runtime description
- queue capability reporting
- resource-state introspection
- ownership-transfer and multi-queue preparation reporting

At the same time, legacy-style backends like DX11 and OpenGL are still supported through compatible adapters.

That hybrid model is the key to understanding the current API: it is backend-neutral, but it is shaped around an increasingly explicit renderer core.
