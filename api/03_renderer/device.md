# Device

`engine::renderer::IDevice`

The central GPU abstraction. `IDevice` creates and destroys GPU resources, manages queues, and provides upload paths for buffer and texture data. All backend-specific code (DX11, OpenGL, Vulkan) lives in separate AddOns — `IDevice` itself has no platform headers.

---

## Creating a device

Devices are created through `DeviceFactory`. Backends register themselves automatically at startup through static initializers — no manual registration is required.

A device is initialized with `IDevice::DeviceDesc`, which controls backend startup behavior such as debug validation, adapter selection, and backend-visible application naming.

Typical device creation flow:

- create the backend device through `DeviceFactory`
- enumerate adapters for the selected backend
- choose an adapter index
- fill `IDevice::DeviceDesc`
- initialize the device

Available backend types: `Null`, `DirectX11`, `OpenGL`, `Vulkan`, `DirectX12`.

---

## Device initialization parameters

`IDevice::DeviceDesc` controls how the backend device is initialized.

### Fields

| Field | Description |
|---|---|
| `enableDebugLayer` | Enables backend-specific debug and validation support when available. This is mainly intended for development builds and is useful for validation messages, backend diagnostics, and graphics API error reporting. |
| `adapterIndex` | Selects the GPU adapter to use. This is typically set from `DeviceFactory::FindBestAdapter(...)` after adapter enumeration. It matters on systems with multiple GPUs. |
| `appName` | Application name passed to the backend for logs, diagnostics, and backend-specific application metadata. This is especially relevant for Vulkan and useful in debugging tools and backend logs. |

### Notes

- `enableDebugLayer` does not change rendering behavior by itself. It only enables diagnostics when supported by the backend.
- `adapterIndex` selects the graphics adapter. It is not a queue index and not a window handle.
- `appName` is not the window title. Window title is configured separately through the platform window description.
- Some backends may ignore parts of `DeviceDesc` if a feature is not supported or not relevant.

---

## Adapter enumeration

Enumerate adapters for the target backend before initialization and choose the adapter you want to use.

`FindBestAdapter` picks the adapter with the highest feature level, preferring discrete GPUs on ties.

This adapter index is then written into `IDevice::DeviceDesc::adapterIndex` before calling `Initialize(...)`.

---

## Swapchain

A swapchain is created from the initialized device and is bound to a native window handle. The swapchain controls presentation, back-buffer count, output format, and vsync behavior.

Typical swapchain configuration includes:

- native window handle
- width and height
- buffer count
- presentation format
- vsync enable/disable

---

## Buffers

Buffers are created through the device with a `BufferDesc` that defines:

- byte size
- buffer type
- usage
- CPU/GPU access policy

Common buffer categories include:

- vertex buffers
- index buffers
- constant/uniform buffers
- staging or upload buffers

GPU-only buffers are typically filled through upload paths such as `UploadBufferData(...)`.

CPU-writable buffers are typically created with CPU write access, then updated through `MapBuffer(...)` / `UnmapBuffer(...)`.

---

## Textures and render targets

Textures are created with a `TextureDesc` describing dimensions, format, usage, and mip count.

Typical texture use cases include:

- sampled textures
- renderable color targets
- depth-stencil textures
- fallback textures
- post-process inputs

Render targets are created separately through `CreateRenderTarget(...)` and usually define:

- width and height
- color format
- depth format

The exact backend resource layout stays hidden behind the device abstraction.

---

## Frame boundaries

Frame submission is controlled through the device frame boundary calls.

Typical flow:

- `BeginFrame()`
- record and submit work
- `EndFrame()`
- present the swapchain

The exact command recording and submission details depend on the active render system and backend, but frame lifetime still begins and ends at the device level.

---

## Destroying resources

Resources created by the device must be destroyed through the device API before shutdown.

Typical resource categories that may need explicit destruction include:

- buffers
- textures
- render targets
- shaders
- pipelines

Always release device-owned resources before calling `device->Shutdown()`.

---

## Diagnostics

The device exposes runtime diagnostics and capability queries.

Typical examples include:

- backend name
- draw call count
- runtime feature support queries

These are useful for debugging, instrumentation, editor overlays, and backend verification.

---

## Summary

`IDevice` is the backend-neutral GPU entry point of the renderer. It is responsible for:

- backend initialization
- adapter selection
- resource creation and destruction
- upload paths
- swapchain creation
- frame lifetime management
- diagnostics and feature queries

`IDevice::DeviceDesc` is the configuration object used during initialization and currently provides the most important startup controls for debug validation, adapter selection, and application naming.
