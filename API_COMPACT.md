# OYNAME ENGINE API OVERVIEW

## 1. Startup Path

Typical direct usage path:

1. Create `GDXEventQueue`
2. Create `WindowDesc`
3. Create `GDXWin32Window`
4. Enumerate DX11 adapters
5. Create DX11 context with `GDXWin32DX11ContextFactory`
6. Create `GDXDX11RenderBackend`
7. Create `GDXECSRenderer`
8. Create `GIDXEngine`
9. Call `Initialize()`
10. Create camera, mesh/material, renderable entities
11. Run with `Run()` or `Step()`

---

## 2. Window and Engine

## `WindowDesc`

Used to configure the Win32 window.

```cpp
struct WindowDesc
{
    int         width      = 1280;
    int         height     = 720;
    std::string title      = "GDX";
    bool        resizable  = true;
    bool        borderless = true;
};
```

### Fields
- `width` — window width
- `height` — window height
- `title` — window title
- `resizable` — whether the window can be resized
- `borderless` — borderless or normal framed window

---

## `IGDXWindow`

Platform-neutral window interface.

```cpp
class IGDXWindow
{
public:
    virtual ~IGDXWindow() = default;

    virtual void        PollEvents() = 0;
    virtual bool        ShouldClose() const = 0;
    virtual int         GetWidth() const = 0;
    virtual int         GetHeight() const = 0;
    virtual bool        GetBorderless() const = 0;
    virtual const char* GetTitle() const = 0;
    virtual void        SetTitle(const char* title) = 0;
};
```

### Functions
- `PollEvents()` — processes pending OS/window messages
- `ShouldClose()` — returns whether the window should close
- `GetWidth()` — current width
- `GetHeight()` — current height
- `GetBorderless()` — borderless state
- `GetTitle()` — current title
- `SetTitle(const char* title)` — sets the window title

---

## `GDXWin32Window`

Concrete Win32 window implementation.

```cpp
class GDXWin32Window final : public IGDXWindow, public IGDXWin32NativeAccess
{
public:
    GDXWin32Window(const WindowDesc& desc, GDXEventQueue& events);
    ~GDXWin32Window() override;

    bool Create();

    void        PollEvents() override;
    bool        ShouldClose() const override;
    int         GetWidth() const override;
    int         GetHeight() const override;
    bool        GetBorderless() const override;
    const char* GetTitle() const override;
    void        SetTitle(const char* title) override;

    bool QueryNativeHandles(GDXWin32NativeHandles& outHandles) const override;
    bool IsBorderless() const override;
};
```

### Functions
- `GDXWin32Window(const WindowDesc& desc, GDXEventQueue& events)` — constructs the window object
- `Create()` — creates the real Win32 window; must succeed before native-handle based context creation
- inherited `IGDXWindow` functions — normal runtime window operations
- `QueryNativeHandles(...)` — exposes Win32 handles for DX11 context creation
- `IsBorderless()` — Win32-side borderless query

---

## `GIDXEngine`

Top-level runtime loop.

```cpp
class GIDXEngine
{
public:
    GIDXEngine(std::unique_ptr<IGDXWindow> window,
              std::unique_ptr<IGDXRenderer> renderer,
              GDXEventQueue& events);

    bool Initialize();
    void Run();
    bool Step();

    float GetDeltaTime() const;
    float GetTotalTime() const;

    using EventFn = std::function<void(const Event&)>;
    void SetEventCallback(EventFn fn);

    void Shutdown();
};
```

### Functions
- `GIDXEngine(std::unique_ptr<IGDXWindow> window, std::unique_ptr<IGDXRenderer> renderer, GDXEventQueue& events)`  
  Creates the engine runtime and takes ownership of window and renderer.
- `Initialize()`  
  Initializes renderer and runtime state.
- `Run()`  
  Runs the main loop until exit.
- `Step()`  
  Runs exactly one frame. Returns `false` when the app should stop.
- `GetDeltaTime()`  
  Returns last frame delta time in seconds.
- `GetTotalTime()`  
  Returns total runtime in seconds.
- `SetEventCallback(EventFn fn)`  
  Registers custom event handling.
- `Shutdown()`  
  Clean shutdown. Safe to call multiple times.

---

## 3. Events and Input

## `Event` types

```cpp
enum class Key
{
    Unknown,
    Escape, Space,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Left, Right, Up, Down
};

struct QuitEvent {};
struct WindowResizedEvent { int width; int height; };
struct KeyPressedEvent { Key key; bool repeat; };
struct KeyReleasedEvent { Key key; };

using Event = std::variant<QuitEvent, WindowResizedEvent, KeyPressedEvent, KeyReleasedEvent>;
```

### Meaning
- `QuitEvent` — application quit request
- `WindowResizedEvent` — window size changed
- `KeyPressedEvent` — key pressed
- `KeyReleasedEvent` — key released

Note:
- ESC is already handled by the engine runtime and triggers shutdown behavior.

---

## `GDXInput`

Per-frame keyboard state API.

```cpp
class GDXInput
{
public:
    static void BeginFrame();
    static void OnEvent(const Event& e);

    static bool KeyDown(Key key);
    static bool KeyHit(Key key);
    static bool KeyReleased(Key key);
};
```

### Functions
- `BeginFrame()` — resets per-frame hit/release state
- `OnEvent(const Event& e)` — feeds an event into the input state
- `KeyDown(Key key)` — key is currently held
- `KeyHit(Key key)` — key was pressed this frame
- `KeyReleased(Key key)` — key was released this frame

---

## 4. DX11 Context Setup

## `GDXDXGIAdapterInfo`

Returned by adapter enumeration.

```cpp
struct GDXDXGIAdapterInfo
{
    unsigned int index;
    std::wstring name;
    size_t       dedicatedVRAM;
    int          featureLevel;
    std::wstring featureLevelName;
};
```

### Fields
- `index` — adapter index used for context creation
- `name` — GPU name
- `dedicatedVRAM` — dedicated VRAM in bytes
- `featureLevel` — encoded DirectX feature level
- `featureLevelName` — readable feature level text

---

## `IGDXDXGIContext`

Created DX11 device/swap-chain context.

```cpp
class IGDXDXGIContext
{
public:
    virtual ~IGDXDXGIContext() = default;

    virtual bool             IsValid() const = 0;
    virtual void             Present(bool vsync) = 0;
    virtual void             Resize(int w, int h) = 0;
    virtual GDXDXGIDeviceInfo QueryDeviceInfo() const = 0;

    virtual ID3D11Device*           GetDevice() const = 0;
    virtual ID3D11DeviceContext*    GetDeviceContext() const = 0;
    virtual ID3D11RenderTargetView* GetRenderTarget() const = 0;
    virtual ID3D11DepthStencilView* GetDepthStencil() const = 0;
};
```

### Functions
- `IsValid()` — whether the DX11 context is usable
- `Present(bool vsync)` — swap/present backbuffer
- `Resize(int w, int h)` — resize backbuffer/depth target
- `QueryDeviceInfo()` — returns active adapter/device info
- `GetDevice()` / `GetDeviceContext()` / `GetRenderTarget()` / `GetDepthStencil()` — raw DX11 objects for backend use

As a normal engine user, you usually create this once and pass it into the DX11 backend.

---

## `GDXWin32DX11ContextFactory`

Creates DX11 contexts for Win32 windows.

```cpp
class GDXWin32DX11ContextFactory
{
public:
    static std::vector<GDXDXGIAdapterInfo> EnumerateAdapters();

    static unsigned int FindBestAdapter(
        const std::vector<GDXDXGIAdapterInfo>& adapters);

    std::unique_ptr<IGDXDXGIContext> Create(
        IGDXWin32NativeAccess& nativeAccess,
        unsigned int adapterIndex) const;
};
```

### Functions
- `EnumerateAdapters()`  
  Returns all available DXGI hardware adapters.
- `FindBestAdapter(const std::vector<GDXDXGIAdapterInfo>& adapters)`  
  Picks the adapter with the best feature level.
- `Create(IGDXWin32NativeAccess& nativeAccess, unsigned int adapterIndex)`  
  Creates the DX11 context for the chosen window and adapter.

---

## 5. Renderer Layer

## `IGDXRenderer`

Generic renderer interface used by `GIDXEngine`.

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

### Functions
- `Initialize()` — initialize renderer resources
- `BeginFrame()` — start frame
- `Tick(float dt)` — per-frame update/render preparation
- `EndFrame()` — finish frame and present
- `Resize(int w, int h)` — resize handling
- `Shutdown()` — clean shutdown

---

## `GDXDX11RenderBackend`

Concrete DX11 backend used by `GDXECSRenderer`.

```cpp
class GDXDX11RenderBackend final : public IGDXRenderBackend
{
public:
    explicit GDXDX11RenderBackend(std::unique_ptr<IGDXDXGIContext> context);
};
```

### Function
- `GDXDX11RenderBackend(std::unique_ptr<IGDXDXGIContext> context)`  
  Wraps the created DX11 context into the engine backend.

As a normal user, this is the backend you instantiate for the direct engine path.

---

## `GDXECSRenderer`

Main user-facing renderer/runtime.

```cpp
class GDXECSRenderer final : public IGDXRenderer
{
public:
    explicit GDXECSRenderer(std::unique_ptr<IGDXRenderBackend> backend);
    ~GDXECSRenderer() override;

    bool Initialize() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Resize(int w, int h) override;
    void Shutdown() override;

    using TickFn = std::function<void(float)>;
    void SetTickCallback(TickFn fn);
    void Tick(float dt);

    Registry& GetRegistry();

    ShaderHandle CreateShader(const std::wstring& vsFile,
                              const std::wstring& psFile,
                              uint32_t vertexFlags = GDX_VERTEX_DEFAULT);

    ShaderHandle CreateShader(const std::wstring& vsFile,
                              const std::wstring& psFile,
                              uint32_t vertexFlags,
                              const GDXShaderLayout& layout);

    TextureHandle LoadTexture(const std::wstring& filePath, bool isSRGB = true);
    TextureHandle CreateTexture(const ImageBuffer& image,
                                const std::wstring& debugName,
                                bool isSRGB = true);

    MeshHandle UploadMesh(MeshAssetResource mesh);
    MaterialHandle CreateMaterial(MaterialResource mat);

    ShaderHandle GetDefaultShader() const;
    void SetShadowMapSize(uint32_t size);
    void SetSceneAmbient(float r, float g, float b);

    RenderTargetHandle CreateRenderTarget(uint32_t w, uint32_t h,
                                          const std::wstring& name,
                                          GDXTextureFormat colorFormat = GDXTextureFormat::RGBA8_UNORM);

    TextureHandle GetRenderTargetTexture(RenderTargetHandle h);

    PostProcessHandle CreatePostProcessPass(const PostProcessPassDesc& desc);
    bool SetPostProcessConstants(PostProcessHandle h, const void* data, uint32_t size);
    bool SetPostProcessEnabled(PostProcessHandle h, bool enabled);
    void ClearPostProcessPasses();

    void SetClearColor(float r, float g, float b, float a = 1.0f);
};
```

### Functions
- `GDXECSRenderer(std::unique_ptr<IGDXRenderBackend> backend)`  
  Creates the main renderer runtime on top of a backend.
- `Initialize()` / `BeginFrame()` / `EndFrame()` / `Resize(...)` / `Shutdown()`  
  Standard renderer lifecycle.
- `SetTickCallback(TickFn fn)`  
  Registers per-frame update code.
- `Tick(float dt)`  
  Executes the update callback and renderer-side per-frame work.
- `GetRegistry()`  
  Returns the ECS registry.
- `CreateShader(...)`  
  Loads and creates shader resources from VS/PS files.
- `LoadTexture(...)`  
  Loads a texture from file.
- `CreateTexture(...)`  
  Creates a texture from CPU image data.
- `UploadMesh(MeshAssetResource mesh)`  
  Uploads a CPU mesh resource and returns a `MeshHandle`.
- `CreateMaterial(MaterialResource mat)`  
  Creates a material and returns a `MaterialHandle`.
- `GetDefaultShader()`  
  Returns the engine default shader handle.
- `SetShadowMapSize(uint32_t size)`  
  Sets the shadow map resolution.
- `SetSceneAmbient(float r, float g, float b)`  
  Sets scene ambient light.
- `CreateRenderTarget(...)`  
  Creates an offscreen render target.
- `GetRenderTargetTexture(...)`  
  Returns the texture belonging to a render target.
- `CreatePostProcessPass(...)`  
  Creates a post-process pass.
- `SetPostProcessConstants(...)`  
  Updates post-process constant buffer data.
- `SetPostProcessEnabled(...)`  
  Enables/disables a post-process pass.
- `ClearPostProcessPasses()`  
  Removes all post-process passes.
- `SetClearColor(...)`  
  Sets the backbuffer clear color.

This is the core class for real engine usage.

---

## 6. ECS Core

## `Registry`

Main ECS storage.

Important public API:

```cpp
EntityID CreateEntity();
void DestroyEntity(EntityID id);
bool IsAlive(EntityID id) const;
size_t EntityCount() const;

template<typename T, typename... Args>
T& Add(EntityID id, Args&&... args);

template<typename T>
T* Get(EntityID id);

template<typename T>
bool Has(EntityID id) const;

template<typename T>
void Remove(EntityID id);

template<typename First, typename... Rest, typename Func>
void View(Func&& func);
```

### Functions
- `CreateEntity()` — creates an entity
- `DestroyEntity(EntityID id)` — destroys an entity
- `IsAlive(EntityID id)` — checks handle validity/aliveness
- `EntityCount()` — number of alive entities
- `Add<T>(...)` — adds a component
- `Get<T>(...)` — gets a component pointer
- `Has<T>(...)` — checks whether the entity has a component
- `Remove<T>(...)` — removes a component
- `View<...>(func)` — iterates over entities that have the specified components

This is the main ECS API you use from gameplay/demo code.

---

## `Handle` types

Resource references are handle-based, not pointer-based.

```cpp
using MeshHandle         = Handle<struct MeshTag>;
using MaterialHandle     = Handle<struct MaterialTag>;
using ShaderHandle       = Handle<struct ShaderTag>;
using TextureHandle      = Handle<struct TextureTag>;
using RenderTargetHandle = Handle<struct RenderTargetTag>;
using PostProcessHandle  = Handle<struct PostProcessTag>;
```

### Meaning
These are typed resource handles used throughout ECS and rendering.

---

## 7. Important ECS Components

These are the components you actually need for normal engine usage.

## `TagComponent`

```cpp
struct TagComponent
{
    std::string name;
};
```

### Purpose
Readable entity name.

---

## `TransformComponent`

```cpp
struct TransformComponent
{
    GIDX::Float3 localPosition;
    GIDX::Float4 localRotation;
    GIDX::Float3 localScale;

    bool dirty;
    uint32_t localVersion;
    uint32_t worldVersion;

    void SetEulerDeg(float pitchDeg, float yawDeg, float rollDeg);
};
```

### Purpose
Stores local transform.

### Important function
- `SetEulerDeg(float pitchDeg, float yawDeg, float rollDeg)` — sets local rotation from Euler angles in degrees

---

## `WorldTransformComponent`

```cpp
struct WorldTransformComponent
{
    GIDX::Float4x4 matrix;
    GIDX::Float4x4 inverse;
};
```

### Purpose
Stores calculated world transform. Required for rendering.

---

## `ParentComponent`

```cpp
struct ParentComponent
{
    EntityID parent = NULL_ENTITY;
};
```

### Purpose
Makes an entity a child of another entity.

---

## `ChildrenComponent`

```cpp
struct ChildrenComponent
{
    std::vector<EntityID> children;
};
```

### Purpose
Tracks direct children of a parent entity.

You normally do not fill this manually unless you explicitly manage hierarchy state yourself.

---

## `RenderableComponent`

```cpp
struct RenderableComponent
{
    MeshHandle mesh;
    MaterialHandle material;
    uint32_t submeshIndex = 0u;
    bool enabled = true;

    bool dirty = true;
    uint32_t stateVersion = 1u;
};
```

### Purpose
Connects an entity to render resources.

### Fields
- `mesh` — mesh resource handle
- `material` — material resource handle
- `submeshIndex` — which submesh to draw
- `enabled` — on/off switch for drawing

---

## `VisibilityComponent`

```cpp
struct VisibilityComponent
{
    bool visible = true;
    bool active = true;
    uint32_t layerMask = 0x00000001u;
    bool castShadows = true;
    bool receiveShadows = true;

    bool dirty = true;
    uint32_t stateVersion = 1u;
};
```

### Purpose
Controls render visibility, layers, and shadow behavior.

---

## `RenderBoundsComponent`

Purpose:
Local bounds used for culling. A renderable entity should have this if you want correct visibility/culling behavior.

---

## `CameraComponent`

```cpp
struct CameraComponent
{
    float fovDeg = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float aspectRatio = 16.0f / 9.0f;

    bool  isOrtho = false;
    float orthoWidth = 10.0f;
    float orthoHeight = 10.0f;

    uint32_t cullMask = 0xFFFFFFFFu;
};
```

### Purpose
Defines camera projection and culling mask.

---

## `ActiveCameraTag`

```cpp
struct ActiveCameraTag {};
```

### Purpose
Marks the active main camera.

---

## `RenderTargetCameraComponent`

```cpp
struct RenderTargetCameraComponent
{
    RenderTargetHandle target = RenderTargetHandle::Invalid();
    bool enabled = true;
    bool autoAspectFromTarget = true;

    bool renderShadows = true;
    bool renderOpaque = true;
    bool renderTransparent = true;

    bool skipSelfReferentialDraws = true;

    RenderPassClearDesc clear;
};
```

### Purpose
Marks a camera that renders into an offscreen render target.

---

## `LightComponent`

```cpp
struct LightComponent
{
    LightKind kind = LightKind::Directional;
    GIDX::Float4 diffuseColor = { 1,1,1,1 };

    float radius = 10.0f;
    float intensity = 1.0f;

    float innerConeAngle = 15.0f;
    float outerConeAngle = 30.0f;

    bool  castShadows = false;
    float shadowOrthoSize = 50.0f;
    float shadowNear = 0.1f;
    float shadowFar = 1000.0f;

    uint32_t affectLayerMask = 0xFFFFFFFFu;
    uint32_t shadowLayerMask = 0xFFFFFFFFu;
};
```

### Purpose
Defines directional, point, or spot light data.

---

## 8. Mesh Data You Need

## `SubmeshData`

CPU-side geometry for one submesh.

```cpp
struct SubmeshData
{
    std::vector<GIDX::Float3> positions;
    std::vector<GIDX::Float3> normals;
    std::vector<GIDX::Float2> uv0;
    std::vector<GIDX::Float2> uv1;
    std::vector<GIDX::Float4> tangents;
    std::vector<GIDX::Float4> colors;
    std::vector<uint32_t>     indices;

    std::vector<GIDX::UInt4>  boneIndices;
    std::vector<GIDX::Float4> boneWeights;

    uint32_t VertexCount() const noexcept;
    uint32_t IndexCount() const noexcept;
    bool HasNormals() const noexcept;
    bool HasUV0() const noexcept;
    bool HasUV1() const noexcept;
    bool HasTangents() const noexcept;
    bool HasSkinning() const noexcept;
    bool IsEmpty() const noexcept;
    uint32_t ComputeVertexFlags() const noexcept;
};
```

### Purpose
This is the CPU mesh data you fill before upload.

### Important rules
- `positions` must be filled
- `indices` may be empty for non-indexed meshes
- optional arrays must match vertex count if used

---

## `MeshAssetResource`

Mesh resource uploaded to the renderer.

```cpp
struct MeshAssetResource
{
    std::vector<SubmeshData>   submeshes;
    std::vector<GpuMeshBuffer> gpuBuffers;

    std::string debugName;
    bool gpuReady = false;

    uint32_t SubmeshCount() const noexcept;
    bool IsEmpty() const noexcept;
    void AddSubmesh(SubmeshData data);
    bool IsGpuReadyAt(uint32_t i) const noexcept;
};
```

### Purpose
Top-level mesh resource containing one or more submeshes.

### Important function
- `AddSubmesh(SubmeshData data)` — appends a submesh before upload

---

## `SubmeshBuilder`

Helper for constructing submesh geometry.

Use this when you want to generate mesh data procedurally instead of filling arrays manually.

---

## `BasicMeshGenerator`

Mesh helper for common primitives.

Use this if you want quick CPU meshes such as cubes or other built-in shapes before uploading them.

---

## 9. Material Data You Need

## `MaterialData`

Core shader/material parameters.

```cpp
struct MaterialData
{
    GIDX::Float4 baseColor;
    GIDX::Float4 specularColor;
    GIDX::Float4 emissiveColor;
    GIDX::Float4 uvTilingOffset;
    GIDX::Float4 uvDetailTilingOffset;
    float metallic;
    float roughness;
    float normalScale;
    float occlusionStrength;
    float shininess;
    float transparency;
    float alphaCutoff;
    float receiveShadows;
    float blendMode;
    float blendFactor;
    uint32_t flags;
};
```

### Purpose
Stores numeric material settings for shading.

---

## `MaterialResource`

Main material object.

```cpp
class MaterialResource
{
public:
    MaterialData data;
    ShaderHandle shader;

    MaterialTextureLayerArray textureLayers{};

    uint32_t sortID = 0u;
    void* gpuConstantBuffer = nullptr;
    bool  cpuDirty = true;
    MaterialShadowCullMode shadowCullMode = MaterialShadowCullMode::Auto;

    bool IsTransparent() const noexcept;
    bool IsAlphaTest() const noexcept;
    bool IsDoubleSided() const noexcept;
    bool IsUnlit() const noexcept;
    bool UsesPBR() const noexcept;
    bool UsesDetailMap() const noexcept;

    void SetShadowCullMode(MaterialShadowCullMode mode) noexcept;
    void SetFlag(MaterialFlags f, bool on) noexcept;

    MaterialTextureLayer& Layer(MaterialTextureSlot slot) noexcept;
    const MaterialTextureLayer& Layer(MaterialTextureSlot slot) const noexcept;

    void SetTexture(MaterialTextureSlot slot, TextureHandle texture,
                    MaterialTextureUVSet uvSet = MaterialTextureUVSet::Auto) noexcept;

    void ClearTexture(MaterialTextureSlot slot) noexcept;
    bool HasTexture(MaterialTextureSlot slot) const noexcept;
    TextureHandle GetTexture(MaterialTextureSlot slot) const noexcept;

    void NormalizeTextureLayers() noexcept;
    void SetDetailTiling(float tilingX, float tilingY,
                         float offsetX = 0.0f, float offsetY = 0.0f) noexcept;
    void SetDetailBlendMode(MaterialTextureBlendMode mode) noexcept;
    MaterialTextureBlendMode GetDetailBlendMode() const noexcept;
    void SetDetailBlendFactor(float factor) noexcept;

    static MaterialResource FlatColor(float r, float g, float b, float a = 1.0f);
};
```

### Important functions
- `SetFlag(MaterialFlags f, bool on)` — enables/disables material features
- `SetTexture(...)` — assigns a texture to a material slot
- `ClearTexture(...)` — removes a texture
- `HasTexture(...)` / `GetTexture(...)` — queries texture assignment
- `SetDetailTiling(...)` — sets detail UV transform
- `SetDetailBlendMode(...)` / `SetDetailBlendFactor(...)` — detail map blending
- `FlatColor(...)` — quick helper for simple flat-color material setup

### Important flags
Examples from `MaterialFlags`:
- `MF_ALPHA_TEST`
- `MF_DOUBLE_SIDED`
- `MF_UNLIT`
- `MF_USE_NORMAL_MAP`
- `MF_USE_ORM_MAP`
- `MF_USE_EMISSIVE`
- `MF_TRANSPARENT`
- `MF_SHADING_PBR`
- `MF_USE_DETAIL_MAP`

---

## 10. What You Need for a Visible Object

For a normal visible renderable entity you typically need:

- `TransformComponent`
- `WorldTransformComponent`
- `RenderableComponent`
- `VisibilityComponent`
- `RenderBoundsComponent`

For the active main camera you typically need:

- `TransformComponent`
- `WorldTransformComponent`
- `CameraComponent`
- `ActiveCameraTag`

If one of these basics is missing, nothing will render.

---

## 11. Minimal Practical Setup

Minimal runtime setup:

```cpp
GDXEventQueue events;

WindowDesc desc{};
auto window = std::make_unique<GDXWin32Window>(desc, events);
window->Create();

auto adapters = GDXWin32DX11ContextFactory::EnumerateAdapters();
unsigned int bestAdapter = GDXWin32DX11ContextFactory::FindBestAdapter(adapters);

GDXWin32DX11ContextFactory factory;
auto dxContext = factory.Create(*window, bestAdapter);

auto backend = std::make_unique<GDXDX11RenderBackend>(std::move(dxContext));
auto renderer = std::make_unique<GDXECSRenderer>(std::move(backend));

GIDXEngine engine(std::move(window), std::move(renderer), events);
engine.Initialize();
engine.Run();
engine.Shutdown();
```

Then:
- fetch `Registry&` from the renderer
- create camera entity
- build `MeshAssetResource`
- create `MaterialResource`
- upload mesh and material
- create renderable entity

---

## 12. Bottom Line

If you want to use the engine as a real user, the core surface is:

- `WindowDesc`
- `GDXWin32Window`
- `GDXWin32DX11ContextFactory`
- `GDXDX11RenderBackend`
- `GDXECSRenderer`
- `GIDXEngine`
- `Registry`
- important ECS components
- `SubmeshData`
- `MeshAssetResource`
- `MaterialResource`

That is the actual direct engine path. Everything beyond that is either helper code, convenience code, or backend internals.
