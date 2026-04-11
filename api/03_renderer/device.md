# Device

`engine::renderer::IDevice`

The central GPU abstraction. `IDevice` creates and destroys GPU resources, manages queues, and provides upload paths for buffer and texture data. All backend-specific code (DX11, OpenGL, Vulkan) lives in separate AddOns — `IDevice` itself has no platform headers.

---

## Creating a device

Devices are created through `DeviceFactory`. Backends register themselves automatically at startup through static initializers — no manual registration is required.

```cpp
using namespace engine::renderer;

auto device = DeviceFactory::Create(DeviceFactory::BackendType::DirectX11);

IDevice::DeviceDesc desc{};
desc.enableDebugLayer = true;
desc.appName          = "MyApp";

if (!device->Initialize(desc))
    return -1;
```

Available backend types: `Null`, `DirectX11`, `OpenGL`, `Vulkan`, `DirectX12`.

---

## Adapter enumeration

```cpp
auto adapters = DeviceFactory::EnumerateAdapters(DeviceFactory::BackendType::DirectX11);
uint32_t best = DeviceFactory::FindBestAdapter(adapters);

desc.adapterIndex = best;
```

`FindBestAdapter` picks the adapter with the highest feature level, preferring discrete GPUs on ties.

---

## Swapchain

```cpp
IDevice::SwapchainDesc sc{};
sc.nativeWindowHandle = window->GetNativeHandle();
sc.width       = 1280u;
sc.height      = 720u;
sc.bufferCount = 2u;
sc.format      = Format::BGRA8_UNORM_SRGB;
sc.vsync       = true;

auto swapchain = device->CreateSwapchain(sc);
```

---

## Buffers

```cpp
BufferDesc vb{};
vb.byteSize = sizeof(vertices);
vb.type     = BufferType::Vertex;
vb.usage    = ResourceUsage::VertexBuffer;
vb.access   = MemoryAccess::GpuOnly;

BufferHandle vbHandle = device->CreateBuffer(vb);
device->UploadBufferData(vbHandle, vertices, sizeof(vertices));
```

CPU-writable buffers:

```cpp
vb.access = MemoryAccess::CpuWrite;
BufferHandle cbHandle = device->CreateBuffer(vb);

void* ptr = device->MapBuffer(cbHandle);
memcpy(ptr, &data, sizeof(data));
device->UnmapBuffer(cbHandle);
```

---

## Textures and render targets

```cpp
TextureDesc td{};
td.width    = 1024u;
td.height   = 1024u;
td.format   = Format::RGBA8_UNORM_SRGB;
td.usage    = ResourceUsage::ShaderResource;
td.mipLevels = 1u;

TextureHandle tex = device->CreateTexture(td);
device->UploadTextureData(tex, pixelData, byteSize);
```

```cpp
RenderTargetDesc rt{};
rt.width       = 1280u;
rt.height      = 720u;
rt.colorFormat = Format::RGBA16_FLOAT;
rt.depthFormat = Format::D24_UNORM_S8_UINT;

RenderTargetHandle rtHandle = device->CreateRenderTarget(rt);
```

---

## Frame boundaries

```cpp
device->BeginFrame();

// record and submit command lists

device->EndFrame();
swapchain->Present(vsync);
```

---

## Destroying resources

```cpp
device->DestroyBuffer(vbHandle);
device->DestroyTexture(tex);
device->DestroyRenderTarget(rtHandle);
device->DestroyShader(shaderHandle);
device->DestroyPipeline(pipelineHandle);
```

Always destroy resources before calling `device->Shutdown()`.

---

## Diagnostics

```cpp
device->GetBackendName()          // "DirectX11", "OpenGL", "Vulkan", ...
device->GetDrawCallCount()        // draw calls in the last frame
device->SupportsFeature("compute") // runtime feature query
```
