# OYNAME ENGINE API REFERENCE

This reference describes how to use the OYNAME Engine API.

The focus is on the direct engine path:
- engine initialization
- window and frame loop
- event system and input
- DX11 adapter enumeration and context creation
- renderer and resource API
- ECS usage
- mesh and material setup

For architecture and high-level usage flow, see the API Overview.

---

## 1. Window and Engine

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
- `borderless` — whether the window is borderless

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
- `PollEvents()` — process pending OS/window messages
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
- `Create()` — creates the native Win32 window
- inherited `IGDXWindow` functions — normal runtime window operations
- `QueryNativeHandles(...)` — exposes Win32 handles needed for DX11 context creation
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

### Purpose
`GIDXEngine` owns the window and renderer and controls the runtime loop.

### Functions

#### `bool Initialize()`
Initializes the engine.

Typical behavior:
- validates window and renderer
- calls `renderer->Initialize()`
- sets initial renderer size
- starts timing state

#### `void Run()`
Runs the main loop until exit.

#### `bool Step()`
Executes exactly one frame.

Typical frame flow:
1. reset input state
2. poll window events
3. compute delta time
4. process events
5. skip rendering if minimized
6. otherwise:
   - `BeginFrame()`
   - `Tick(dt)`
   - `EndFrame()`

Returns:
- `true` if execution should continue
- `false` if the application should stop

#### `float GetDeltaTime() const`
Returns the last frame delta time in seconds.

#### `float GetTotalTime() const`
Returns total runtime in seconds.

#### `void SetEventCallback(EventFn fn)`
Registers custom event handling.

#### `void Shutdown()`
Clean shutdown. Safe to call multiple times.

---

## 2. Events and Input

## Event Types

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

### ESC behavior
ESC is already handled by the engine runtime and triggers shutdown behavior.

---

## `GDXEventQueue`

Event queue written by platform code and consumed by the engine once per frame.

### Purpose
- collect events
- provide consistent per-frame processing

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
- `OnEvent(const Event& e)` — feeds an event into input state
- `KeyDown(Key key)` — key is currently held
- `KeyHit(Key key)` — key was pressed this frame
- `KeyReleased(Key key)` — key was released this frame

### Example

```cpp
if (GDXInput::KeyDown(Key::Left))
{
    // continuous movement
}

if (GDXInput::KeyHit(Key::Space))
{
    // one-shot action
}
```

---

## 3. DX11 Context Setup

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

    virtual bool              IsValid() const = 0;
    virtual void              Present(bool vsync) = 0;
    virtual void              Resize(int w, int h) = 0;
    virtual GDXDXGIDeviceInfo QueryDeviceInfo() const = 0;

    virtual ID3D11Device*           GetDevice() const = 0;
    virtual ID3D11DeviceContext*    GetDeviceContext() const = 0;
    virtual ID3D11RenderTargetView* GetRenderTarget() const = 0;
    virtual ID3D11DepthStencilView* GetDepthStencil() const = 0;
};
```

### Functions
- `IsValid()` — whether the DX11 context is usable
- `Present(bool vsync)` — presents the backbuffer
- `Resize(int w, int h)` — resizes backbuffer/depth targets
- `QueryDeviceInfo()` — returns active device/adapter info
- `GetDevice()` / `GetDeviceContext()` / `GetRenderTarget()` / `GetDepthStencil()` — raw DX11 objects for backend use

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
- `EnumerateAdapters()` — returns available hardware adapters
- `FindBestAdapter(...)` — picks the adapter with the highest feature level
- `Create(...)` — creates the DX11 context for the selected window and adapter

### Return value
- valid `IGDXDXGIContext` on success
- `nullptr` on failure

---

## 4. Renderer Layer

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
- `Tick(float dt)` — per-frame update/render work
- `EndFrame()` — finish frame and present
- `Resize(int w, int h)` — resize handling
- `Shutdown()` — clean shutdown

---

## `GDXDX11RenderBackend`

Concrete DX11 backend.

```cpp
class GDXDX11RenderBackend final : public IGDXRenderBackend
{
public:
    explicit GDXDX11RenderBackend(std::unique_ptr<IGDXDXGIContext> context);
};
```

### Function
- `GDXDX11RenderBackend(...)` — wraps the DX11 context into an engine backend

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

    ResourceStore<MeshAssetResource, MeshTag>& GetMeshStore();
    ResourceStore<MaterialResource, MaterialTag>& GetMatStore();
    ResourceStore<GDXShaderResource, ShaderTag>& GetShaderStore();
    ResourceStore<GDXTextureResource, TextureTag>& GetTextureStore();

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

### Responsibilities
- renderer lifecycle
- ECS registry access
- resource creation
- render targets
- post-processing
- per-frame callback execution

### Important functions
- `SetTickCallback(TickFn fn)` — registers per-frame update code
- `GetRegistry()` — returns ECS registry
- `CreateShader(...)` — creates shader resources
- `LoadTexture(...)` / `CreateTexture(...)` — creates textures
- `UploadMesh(...)` — uploads a CPU mesh resource
- `CreateMaterial(...)` — creates a material resource
- `GetDefaultShader()` — returns default shader handle
- `SetShadowMapSize(...)` — configures shadow map resolution
- `SetSceneAmbient(...)` — configures ambient scene light
- `CreateRenderTarget(...)` — creates an offscreen render target
- `GetRenderTargetTexture(...)` — returns the texture of a render target
- `CreatePostProcessPass(...)` — creates a post-process pass
- `SetPostProcessConstants(...)` — updates post-process constant data
- `SetPostProcessEnabled(...)` — enables/disables a post-process pass
- `ClearPostProcessPasses()` — removes all post-process passes
- `SetClearColor(...)` — sets backbuffer clear color

---

## 5. ECS Core

## `Registry`

Main ECS storage.

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
- `IsAlive(EntityID id)` — checks whether an entity is valid/alive
- `EntityCount()` — number of alive entities
- `Add<T>(...)` — adds a component
- `Get<T>(...)` — gets a component pointer
- `Has<T>(...)` — checks whether the entity has a component
- `Remove<T>(...)` — removes a component
- `View<...>(func)` — iterates over entities that have the specified components

### Example

```cpp
Registry& registry = renderer.GetRegistry();

EntityID entity = registry.CreateEntity();
registry.Add<TagComponent>(entity, TagComponent{"Triangle"});
```

---

## Handle Types

Resource references are handle-based.

```cpp
using MeshHandle         = Handle<struct MeshTag>;
using MaterialHandle     = Handle<struct MaterialTag>;
using ShaderHandle       = Handle<struct ShaderTag>;
using TextureHandle      = Handle<struct TextureTag>;
using RenderTargetHandle = Handle<struct RenderTargetTag>;
using PostProcessHandle  = Handle<struct PostProcessTag>;
```

---

## 6. Important ECS Components

## `TagComponent`

```cpp
struct TagComponent
{
    std::string name;
};
```

### Purpose
Human-readable entity name.

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
- `SetEulerDeg(...)` — sets local rotation from Euler angles in degrees

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
Stores calculated world transform and inverse.

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
Controls render visibility, activity, layers, and shadow behavior.

---

## `RenderBoundsComponent`

### Purpose
Local bounds used for culling.

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

## 7. Mesh Data

## `SubmeshData`

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
CPU-side geometry for one submesh.

### Rules
- `positions` must be filled
- `indices` may be empty for non-indexed meshes
- optional arrays must match vertex count if used

---

## `MeshAssetResource`

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

## 8. Material Data

## `MaterialData`

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
Numeric material parameters for shading.

---

## `MaterialResource`

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
- `SetFlag(...)` — enables/disables material features
- `SetTexture(...)` — assigns a texture to a slot
- `ClearTexture(...)` — removes a texture
- `HasTexture(...)` / `GetTexture(...)` — query assigned textures
- `SetDetailTiling(...)` — set detail UV transform
- `SetDetailBlendMode(...)` / `SetDetailBlendFactor(...)` — detail map blending
- `FlatColor(...)` — helper for simple flat-color setup

### Example flags
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

## 9. Resource Creation Examples

### Shader

```cpp
ShaderHandle shader = renderer.CreateShader(
    L"shader/ECSVertexShader.hlsl",
    L"shader/ECSPixelShader.hlsl",
    GDX_VERTEX_DEFAULT);
```

### Texture from file

```cpp
TextureHandle tex = renderer.LoadTexture(L"assets/albedo.png", true);
```

### Texture from CPU image

```cpp
TextureHandle tex = renderer.CreateTexture(imageBuffer, L"GeneratedTexture", true);
```

### Mesh

```cpp
MeshAssetResource mesh = /* build mesh data */;
MeshHandle meshHandle = renderer.UploadMesh(std::move(mesh));
```

### Material

```cpp
MaterialResource mat{};
MaterialHandle matHandle = renderer.CreateMaterial(std::move(mat));
```

---

## 10. Render Targets and Post-Processing

### Create render target

```cpp
RenderTargetHandle rt = renderer.CreateRenderTarget(
    1024,
    1024,
    L"OffscreenColor",
    GDXTextureFormat::RGBA8_UNORM);
```

### Get render target texture

```cpp
TextureHandle rtTexture = renderer.GetRenderTargetTexture(rt);
```

### Create post-process pass

```cpp
PostProcessPassDesc desc{};
PostProcessHandle pp = renderer.CreatePostProcessPass(desc);
```

### Set constants

```cpp
renderer.SetPostProcessConstants(pp, &myData, sizeof(myData));
```

### Enable/disable

```cpp
renderer.SetPostProcessEnabled(pp, true);
```

### Clear all passes

```cpp
renderer.ClearPostProcessPasses();
```

---

## 11. Minimal Initialization Example

```cpp
int main()
{
    GDXEventQueue events;

    WindowDesc desc{};
    desc.width = 1280;
    desc.height = 720;
    desc.title = "GIDX Demo";

    auto window = std::make_unique<GDXWin32Window>(desc, events);
    window->Create();

    auto adapters = GDXWin32DX11ContextFactory::EnumerateAdapters();
    if (adapters.empty())
        return 1;

    unsigned int bestAdapter = GDXWin32DX11ContextFactory::FindBestAdapter(adapters);

    GDXWin32DX11ContextFactory factory;
    auto dxContext = factory.Create(*window, bestAdapter);
    if (!dxContext)
        return 1;

    auto backend = std::make_unique<GDXDX11RenderBackend>(std::move(dxContext));
    auto renderer = std::make_unique<GDXECSRenderer>(std::move(backend));

    GIDXEngine engine(std::move(window), std::move(renderer), events);

    if (!engine.Initialize())
        return 1;

    engine.Run();
    engine.Shutdown();
    return 0;
}
```

---

## 12. Minimal ECS Render Path

For a visible renderable entity you typically need:

- `TransformComponent`
- `WorldTransformComponent`
- `RenderableComponent`
- `VisibilityComponent`
- `RenderBoundsComponent`

For an active camera:

- `TransformComponent`
- `WorldTransformComponent`
- `CameraComponent`
- `ActiveCameraTag`
