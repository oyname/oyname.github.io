# GIDX Engine API Overview (derived from ZIP snapshot)

This overview is based on the uploaded snapshot `gidx26 framegraph readonly snapshot.zip`.
It summarizes the public engine-facing API and the main convenience layer in `gidx.h`.

## 1. Core runtime

### `GDXEngine`
Controls lifecycle, frame loop, event processing, and timing.

- `GDXEngine(std::unique_ptr<IGDXWindow> window, std::unique_ptr<IGDXRenderer> renderer, GDXEventQueue& events)`  
  Creates the engine and takes ownership of `window` and `renderer`.
- `bool Initialize()`  
  Initializes engine state. Returns `true` on success.
- `void Run()`  
  Runs the main loop.
- `bool Step()`  
  Executes one frame. Returns `false` when the app should stop.
- `float GetDeltaTime() const`  
  Returns the last frame delta time in seconds.
- `float GetTotalTime() const`  
  Returns total runtime since start.
- `void SetEventCallback(EventFn fn)`  
  Registers a callback `std::function<void(const Event&)>`.
- `void Shutdown()`  
  Clean shutdown, documented as idempotent.

## 2. Renderer API

### `IGDXRenderer`
Minimal renderer interface used by `GDXEngine`.

- `bool Initialize()`
- `void BeginFrame()`
- `void Tick(float dt)` — `dt`: frame delta time
- `void EndFrame()`
- `void Resize(int w, int h)` — `w`, `h`: viewport size
- `void Shutdown()`

### `GDXECSRenderer`
Main high-level runtime. Exposes ECS, resources, RTT, post-processing, and frame execution.

#### Lifecycle
- `explicit GDXECSRenderer(std::unique_ptr<IGDXRenderBackend> backend)` — wraps a backend
- `bool Initialize()`
- `void BeginFrame()`
- `void EndFrame()`
- `void Resize(int w, int h)`
- `void Shutdown()`
- `void Tick(float dt)`
- `void SetTickCallback(TickFn fn)` — per-frame callback

#### ECS access
- `Registry& GetRegistry()` — returns the ECS registry

#### Shader API
- `ShaderHandle CreateShader(const std::wstring& vsFile, const std::wstring& psFile, uint32_t vertexFlags = GDX_VERTEX_DEFAULT)`
- `ShaderHandle CreateShader(const std::wstring& vsFile, const std::wstring& psFile, uint32_t vertexFlags, const GDXShaderLayout& layout)`
- `ShaderHandle GetDefaultShader() const`

#### Texture API
- `TextureHandle LoadTexture(const std::wstring& filePath, bool isSRGB = true)`
- `TextureHandle CreateTexture(const ImageBuffer& image, const std::wstring& debugName, bool isSRGB = true)`

#### Mesh/material API
- `MeshHandle UploadMesh(MeshAssetResource mesh)`
- `MaterialHandle CreateMaterial(MaterialResource mat)`

#### Renderer configuration
- `void SetShadowMapSize(uint32_t size)`
- `void SetSceneAmbient(float r, float g, float b)`
- `void SetClearColor(float r, float g, float b, float a = 1.0f)`
- `const FrameStats& GetFrameStats() const`
- `void SetDebugCullingOptions(const DebugCullingOptions& options)`
- `const DebugCullingOptions& GetDebugCullingOptions() const`

#### Resource store access
- `GetMeshStore()`
- `GetMatStore()`
- `GetShaderStore()`
- `GetTextureStore()`

#### Render target API
- `RenderTargetHandle CreateRenderTarget(uint32_t w, uint32_t h, const std::wstring& name, GDXTextureFormat colorFormat = GDXTextureFormat::RGBA8_UNORM)`
- `TextureHandle GetRenderTargetTexture(RenderTargetHandle h)`

#### Post-process API
- `PostProcessHandle CreatePostProcessPass(const PostProcessPassDesc& desc)`
- `bool SetPostProcessConstants(PostProcessHandle h, const void* data, uint32_t size)`
- `bool SetPostProcessEnabled(PostProcessHandle h, bool enabled)`
- `void ClearPostProcessPasses()`

## 3. Backend interface

### `IGDXRenderBackend`
Contract implemented by DX11/OpenGL/DX12 backends.

Engine-relevant methods:
- `bool Initialize(ResourceStore<GDXTextureResource, TextureTag>& texStore)`
- `void BeginFrame(const float clearColor[4])`
- `void Present(bool vsync)`
- `void Resize(int w, int h)`
- `bool UploadMesh(MeshAssetResource& mesh)`
- `bool CreateMaterialGpu(MaterialResource& mat)`
- `void UpdateLights(Registry& registry, FrameData& frame)`
- `void UpdateFrameConstants(const FrameData& frame)`
- `uint32_t GetDrawCallCount() const`
- `bool HasShadowResources() const`
- `const DefaultTextureSet& GetDefaultTextures() const`

## 4. Windowing / platform

### `IGDXWindow`
Platform-neutral window abstraction.

- `void PollEvents()`
- `bool ShouldClose() const`
- `int GetWidth() const`
- `int GetHeight() const`
- `bool GetBorderless() const`
- `const char* GetTitle() const`
- `void SetTitle(const char* title)`

### `GDXWin32Window`
Win32 implementation.

- `GDXWin32Window(const WindowDesc& desc, GDXEventQueue& events)`
- `~GDXWin32Window()`
- `bool Create()` — creates the native window
- plus all `IGDXWindow` methods
- `bool QueryNativeHandles(GDXWin32NativeHandles& outHandles) const`
- `bool IsBorderless() const`

### `IGDXWin32NativeAccess`
Secondary interface for raw Win32 handles.

- `bool QueryNativeHandles(GDXWin32NativeHandles& outHandles) const`
- `bool IsBorderless() const`

### `GDXWin32DX11ContextFactory`
Creates DX11 context/device for a Win32 window.

- `static std::vector<GDXDXGIAdapterInfo> EnumerateAdapters()`
- `static unsigned int FindBestAdapter(const std::vector<GDXDXGIAdapterInfo>& adapters)`
- `std::unique_ptr<IGDXDXGIContext> Create(IGDXWin32NativeAccess& nativeAccess, unsigned int adapterIndex) const`

### `WindowDesc`
Simple window config struct.

Public fields:
- `int width`
- `int height`
- `const char* title`
- `bool resizable`
- `bool borderless`

## 5. Events / input

### Event types
- `enum class Key` — `Unknown`, `Escape`, `Space`, `A..Z`, `Left`, `Right`, `Up`, `Down`
- `QuitEvent`
- `WindowResizedEvent { int width; int height; }`
- `KeyPressedEvent { Key key; bool repeat; }`
- `KeyReleasedEvent { Key key; }`
- `using Event = std::variant<...>`

### `GDXEventQueue`
Thread-safe engine event queue.

- `void Push(const Event& e)`
- `std::vector<Event> SnapshotAndClear()`

### `GDXInput`
Static keyboard state tracking.

- `static void BeginFrame()`
- `static void OnEvent(const Event& e)`
- `static bool KeyDown(Key key)`
- `static bool KeyHit(Key key)`
- `static bool KeyReleased(Key key)`

## 6. ECS registry

### `Registry`
Compact entity/component storage.

#### Entity management
- `EntityID CreateEntity()`
- `void DestroyEntity(EntityID id)`
- `bool IsAlive(EntityID id) const`
- `size_t EntityCount() const`

#### Component operations
- `template<typename T, typename... Args> T& Add(EntityID id, Args&&... args)`
- `template<typename T> T& Add(EntityID id, T&& value)`
- `template<typename T> T* Get(EntityID id)`
- `template<typename T> const T* Get(EntityID id) const`
- `template<typename T> bool Has(EntityID id) const`
- `template<typename T> void Remove(EntityID id)`
- `template<typename T> void MarkChanged(EntityID id)`

#### Views
- `template<typename First, typename... Rest, typename Func> void View(Func&& func)`
- const overload available

## 7. Important ECS components

- `TagComponent` — readable entity name
- `TransformComponent` — local position/rotation/scale, dirty flags, `SetEulerDeg(...)`
- `WorldTransformComponent` — world matrix and inverse
- `ParentComponent` — parent entity reference
- `ChildrenComponent` — direct child list, `Count()`, `Add()`, `Remove()`
- `RenderableComponent` — `mesh`, `material`, `submeshIndex`, `enabled`, dirty/version fields
- `SkinComponent` — runtime bone matrices
- `VisibilityComponent` — `visible`, `active`, `layerMask`, shadow flags
- `RenderBoundsComponent` — sphere/AABB culling data
- `CameraComponent` — projection settings
- `LightComponent` — light parameters for directional/point/spot lights

## 8. Mesh helpers / resource structs

### `SubmeshData`
CPU-side geometry data.

Fields:
- `positions`, `normals`, `uv0`, `uv1`, `tangents`, `colors`, `indices`, `boneIndices`, `boneWeights`

Methods:
- `VertexCount()`
- `IndexCount()`
- `HasNormals()`
- `HasUV0()`
- `HasUV1()`
- `HasTangents()`
- `HasSkinning()`
- `IsEmpty()`
- `ComputeVertexFlags()`
- `Validate()`

### `BuiltinMeshes`
Header-defined mesh creators in `SubmeshData.h`.
Visible built-ins include:
- `Triangle()`
- `Cube()`

### `MeshAssetResource`
Shared mesh resource.

Fields:
- `submeshes`
- `gpuBuffers`
- `debugName`
- `gpuReady`
- `gpuReleaseCallback`

Methods:
- `SubmeshCount()`
- `IsEmpty()`
- `AddSubmesh(SubmeshData data)`
- `IsGpuReadyAt(uint32_t i)`

### `SubmeshBuilder`
Convenience CPU geometry builder.

- `Clear()`
- `Reserve(uint32_t vertexCount, uint32_t indexCount = 0u)`
- `AddVertex(...)`
- `VertexCount()`
- `IndexCount()`
- `SetNormal(...)`
- `SetUV0(...)`
- `SetUV1(...)`
- `SetColor(...)`
- `SetTangent(...)`
- `SetBoneData(...)`
- `AddIndex(uint32_t i)`
- `AddTriangle(uint32_t i0, uint32_t i1, uint32_t i2)`
- `AddQuad(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3)`
- `Peek()`
- `Build()`
- `MoveBuild()`

### `BasicMeshGenerator`
Ready-made CPU mesh helpers.

- `Finalize(SubmeshData& s, const BasicMeshBuildOptions& opt)`
- `QuadXY(float width = 1.0f, float height = 1.0f, const BasicMeshBuildOptions& opt = {})`
- `GridXZ(uint32_t cellsX, uint32_t cellsZ, float sizeX = 1.0f, float sizeZ = 1.0f, const BasicMeshBuildOptions& opt = {})`
- `MakeMeshAsset(SubmeshData submesh, const std::string& debugName = {})`

## 9. Material API

### `MaterialResource`
Material state object.

Important public members / helpers:
- fields: `data`, `shader`, `textureLayers`, `sortID`, `gpuConstantBuffer`, `cpuDirty`, `shadowCullMode`
- queries: `IsTransparent()`, `IsAlphaTest()`, `IsDoubleSided()`, `IsUnlit()`, `UsesPBR()`, `UsesDetailMap()`
- shadow: `GetShadowCullMode()`, `SetShadowCullMode(...)`, `IsShadowDoubleSided()`
- flags: `SetFlag(MaterialFlags f, bool on)`
- textures: `Layer(...)`, `SetTexture(...)`, `ClearTexture(...)`, `HasTexture(...)`, `GetTexture(...)`
- normalization: `HasConsistentTextureState()`, `NormalizeTextureLayers()`
- detail settings: `SetDetailTiling(...)`, `SetDetailBlendMode(...)`, `GetDetailBlendMode()`, `SetDetailBlendFactor(float factor)`
- helper: `static MaterialResource FlatColor(float r, float g, float b, float a = 1.0f)`

## 10. DX11 texture loader helpers

Low-level functions in `GDXTextureLoader.h`:
- `GDXTextureLoader_LoadFromFile(...)`
- `GDXTextureLoader_Create1x1(...)`
- `GDXTextureLoader_CreateFromImage(...)`

These are backend-level helpers, not the preferred high-level runtime path.

## 11. Job system

### `JobSystem`
Simple worker-based parallel-for system.

- `JobSystem()`
- `explicit JobSystem(uint32_t workerCount)`
- `~JobSystem()`
- `void Initialize(uint32_t workerCount = 0u)`
- `void Shutdown()`
- `uint32_t GetWorkerCount() const noexcept`
- `bool IsParallel() const noexcept`
- `void ParallelFor(size_t itemCount, const std::function<void(size_t begin, size_t end)>& fn, size_t minBatchSize = 64u)`

## 12. Convenience layer in `gidx.h`

This is the simplified façade over the direct engine path.

### Bootstrap / loop
- `bool Engine::Graphics(int backend, int width, int height, const char* title = "OYNAME Engine", float clearR = 0.04f, float clearG = 0.04f, float clearB = 0.10f, bool resizable = true)`
- `void Engine::Bind(GDXECSRenderer& r, GDXEngine& e)`
- `void Engine::OnUpdate(TickFn fn)`
- `void Engine::OnEvent(EventFn fn)`
- `void Engine::Run()`
- `void Engine::Quit()`
- `bool Engine::AppRunning()`
- `float Engine::DeltaTime()`
- `bool Engine::Frame()`

### Input wrapper
- `bool Engine::Input::KeyDown(Key key)`
- `bool Engine::Input::KeyHit(Key key)`
- `bool Engine::Input::KeyReleased(Key key)`

### Mesh upload / built-ins
- `LPMESH UploadMeshAsset(MeshAssetResource asset, const MeshBuildSettings& settings = {})`
- `LPMESH UploadSubmesh(SubmeshData submesh, const char* debugName = "Submesh", const MeshBuildSettings& settings = {})`
- `LPMESH Sphere(float radius = 0.5f, uint32_t slices = 24, uint32_t stacks = 16)`
- `LPMESH Cube()`
- `LPMESH Octahedron(float radius = 0.5f)`

### Entity creation
- `void CreateMesh(LPENTITY* out, LPMESH mesh = NULL_MESH, const char* tag = "")`
- `void CreateCamera(LPENTITY* out, float fovDeg = 60.0f, float nearPlane = 0.1f, float farPlane = 1000.0f, const char* tag = "Camera")`
- `void CreateLight(LPENTITY* out, LightKind kind = LightKind::Directional, float r = 1.0f, float g = 1.0f, float b = 1.0f, const char* tag = "Light")`

### Transform helpers
- `void PositionEntity(LPENTITY e, float x, float y, float z)`
- `void RotateEntity(LPENTITY e, float pitchDeg, float yawDeg, float rollDeg)`
- `void TurnEntity(LPENTITY e, float pitchDeg, float yawDeg, float rollDeg)`
- `void ScaleEntity(LPENTITY e, float x, float y, float z)`
- `void LookAt(LPENTITY e, float tx, float ty, float tz)`
- `float EntityX(LPENTITY e)`
- `float EntityY(LPENTITY e)`
- `float EntityZ(LPENTITY e)`

### Renderable assignment
- `void AssignMesh(LPENTITY e, LPMESH mesh, uint32_t submeshIndex = 0u)`
- `void AssignMaterial(LPENTITY e, LPMATERIAL mat)`

### Material helpers
- `void CreateMaterial(LPMATERIAL* out, float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)`
- `void MaterialColor(LPMATERIAL mat, float r, float g, float b, float a = 1.0f)`
- `void MaterialMetallic(LPMATERIAL mat, float metallic)`
- `void MaterialRoughness(LPMATERIAL mat, float roughness)`
- `void MaterialPBR(LPMATERIAL mat, float metallic, float roughness)`

## 13. What matters in practice

The real primary API is:
1. `GDXEngine`
2. `GDXECSRenderer`
3. `Registry`
4. `GDXWin32Window` + `GDXWin32DX11ContextFactory`
5. resource structs like `MeshAssetResource`, `SubmeshData`, `MaterialResource`

`gidx.h` is a convenience layer on top, not the core architecture.
