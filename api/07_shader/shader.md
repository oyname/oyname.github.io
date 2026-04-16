# KROM Engine Shader System API Reference

Version: extracted from current project source  
Scope: shader assets, compilation targets, variant system, binding model, runtime material/shader preparation  
Audience: engine/runtime developers

---

## 1. Purpose

The KROM shader system is split into five layers:

1. **CPU asset layer**: stores logical shader source and compiled artifacts.
2. **target resolution layer**: maps the active backend device to a shader target profile.
3. **compiler layer**: compiles a shader asset for a target profile and optional variant defines.
4. **variant cache layer**: memoizes compiled shader variants and uploads them to GPU.
5. **runtime layer**: prepares GPU shader handles and material GPU state, then binds them to command lists.

This document describes the public API and the engine contract exposed by these layers.

---

## 2. Source File Layout

### Public headers

- `include/assets/AssetRegistry.hpp`
- `include/renderer/ShaderCompiler.hpp`
- `include/renderer/ShaderContract.hpp`
- `include/renderer/ShaderBindingModel.hpp`
- `include/renderer/ShaderRuntime.hpp`
- `include/renderer/ShaderVariantCache.hpp`
- `include/renderer/RendererTypes.hpp`

### Primary implementations

- `src/renderer/ShaderCompiler.cpp`
- `src/renderer/ShaderRuntimeAssets.cpp`
- `src/renderer/ShaderRuntimeMaterials.cpp`
- `src/renderer/ShaderRuntimeResources.cpp`
- `src/renderer/ShaderCompilerD3D.cpp`
- `src/renderer/ShaderCompilerDXC.cpp`
- `src/renderer/ShaderCompilerGLSL.cpp`
- `src/renderer/ShaderCompilerSPIRV.cpp`
- `src/renderer/ShaderCompilerShared.cpp`

### Project shader source directory

- `assets/`

Observed logical shader families:

- `fullscreen`
- `passthrough`
- `pbr_lit`
- `quad_unlit`
- `triangle_color`

Observed file naming convention:

| Pattern | Meaning |
|---|---|
| `name.hlslvs` | HLSL vertex shader source |
| `name.hlslps` | HLSL pixel shader source |
| `name.vert` | GLSL vertex shader source |
| `name.frag` | GLSL fragment shader source |
| `name_vk.vert` | Vulkan-target GLSL vertex shader source |
| `name_vk.frag` | Vulkan-target GLSL fragment shader source |

This naming is a **project asset convention**, not a C++ API type.

---

## 3. Core Asset Types

Defined in `include/assets/AssetRegistry.hpp`.

### 3.1 `engine::assets::ShaderStage`

```cpp
enum class ShaderStage : uint8_t
{
    Vertex   = 0,
    Fragment = 1,
    Compute  = 2,
    Geometry = 3,
    Hull     = 4,
    Domain   = 5,
};
```

Represents the logical shader stage of a shader asset or compiled artifact.

### 3.2 `engine::assets::ShaderSourceLanguage`

```cpp
enum class ShaderSourceLanguage : uint8_t
{
    Unknown = 0,
    HLSL,
    GLSL,
    WGSL,
};
```

Represents the source language stored in the asset.

### 3.3 `engine::assets::ShaderTargetProfile`

```cpp
enum class ShaderTargetProfile : uint8_t
{
    Generic = 0,
    Null,
    DirectX11_SM5,
    DirectX12_SM6,
    Vulkan_SPIRV,
    OpenGL_GLSL450,
};
```

Represents the concrete runtime compilation target.

### 3.4 `engine::assets::ShaderDependencyRecord`

```cpp
struct ShaderDependencyRecord
{
    std::string path;
    uint64_t    contentHash = 0ull;
};
```

Describes one dependency used when building a compiled artifact. Intended for cache invalidation and dependency tracking.

### 3.5 `engine::assets::CompiledShaderArtifact`

```cpp
struct CompiledShaderArtifact
{
    ShaderTargetProfile  target = ShaderTargetProfile::Generic;
    ShaderStage          stage = ShaderStage::Vertex;
    std::string          entryPoint = "main";
    std::string          debugName;
    std::vector<uint8_t> bytecode;
    std::string          sourceText;
    uint64_t             sourceHash = 0ull;
    std::vector<std::string> defines;
    std::string          cacheKey;
    uint32_t             cacheSchemaVersion = 0u;
    std::vector<ShaderDependencyRecord> dependencies;
    engine::renderer::ShaderPipelineContract contract;

    [[nodiscard]] bool IsValid() const noexcept;
};
```

#### Field semantics

| Field | Meaning |
|---|---|
| `target` | compiled target profile |
| `stage` | shader stage for the artifact |
| `entryPoint` | compiler entry point |
| `debugName` | debug label used for logs or diagnostics |
| `bytecode` | binary payload such as DXBC, DXIL, SPIR-V |
| `sourceText` | source payload used when the runtime consumes text directly |
| `sourceHash` | content hash of the source input |
| `defines` | active preprocessor define set used for compilation |
| `cacheKey` | key for compiler cache lookup |
| `cacheSchemaVersion` | cache format version |
| `dependencies` | source/include dependency records |
| `contract` | binding/interface contract extracted for runtime use |

#### Validity rule

`IsValid()` returns `true` when either `bytecode` or `sourceText` is non-empty.

### 3.6 `engine::assets::ShaderAsset`

```cpp
struct ShaderAsset : AssetBase
{
    ShaderStage                          stage = ShaderStage::Vertex;
    ShaderSourceLanguage                 sourceLanguage = ShaderSourceLanguage::Unknown;
    std::string                          entryPoint = "main";
    std::string                          sourceCode;
    std::string                          resolvedPath;
    std::vector<uint8_t>                 bytecode;
    std::vector<CompiledShaderArtifact>  compiledArtifacts;
    GpuUploadStatus                      gpuStatus{};
};
```

#### Field semantics

| Field | Meaning |
|---|---|
| `stage` | logical shader stage |
| `sourceLanguage` | declared input language |
| `entryPoint` | source entry point |
| `sourceCode` | logical source text |
| `resolvedPath` | resolved source path for include and cache logic |
| `bytecode` | legacy fallback bytecode payload |
| `compiledArtifacts` | target-specific compiled outputs |
| `gpuStatus` | upload state tracking |

#### Notes

- `ShaderAsset` is the **CPU-side canonical shader record**.
- A single shader asset may hold multiple `compiledArtifacts`, one per target or define set.
- The runtime may compile a new artifact if the required target is missing.

---

## 4. Compiler API

Defined in `include/renderer/ShaderCompiler.hpp`.

### 4.1 Class synopsis

```cpp
class ShaderCompiler
{
public:
    [[nodiscard]] static assets::ShaderTargetProfile ResolveTargetProfile(const IDevice& device);
    [[nodiscard]] static ShaderTargetApi ResolveTargetApi(const IDevice& device);
    [[nodiscard]] static ShaderBinaryFormat ResolveBinaryFormat(assets::ShaderTargetProfile profile) noexcept;
    [[nodiscard]] static const char* ToString(assets::ShaderTargetProfile profile) noexcept;
    [[nodiscard]] static bool IsRuntimeConsumable(const assets::CompiledShaderArtifact& shader) noexcept;
    [[nodiscard]] static bool CompileForTarget(const assets::ShaderAsset& asset,
                                               assets::ShaderTargetProfile target,
                                               assets::CompiledShaderArtifact& outCompiled,
                                               std::string* outError = nullptr);
    [[nodiscard]] static std::vector<std::string> VariantFlagsToDefines(ShaderVariantFlag flags) noexcept;
    [[nodiscard]] static bool CompileVariant(const assets::ShaderAsset& asset,
                                             assets::ShaderTargetProfile target,
                                             ShaderVariantFlag flags,
                                             assets::CompiledShaderArtifact& outCompiled,
                                             std::string* outError = nullptr);
};
```

### 4.2 `ResolveTargetProfile`

```cpp
static assets::ShaderTargetProfile ResolveTargetProfile(const IDevice& device);
```

Returns the shader target profile exposed by the active device.

#### Source behavior

Implementation delegates directly to:

```cpp
device.GetShaderTargetProfile()
```

This means target selection is **device-owned**, not inferred by string matching or shader filename parsing.

### 4.3 `ResolveTargetApi`

```cpp
static ShaderTargetApi ResolveTargetApi(const IDevice& device);
```

Maps the device target profile to an API namespace.

#### Mapping

| Target profile | Target API |
|---|---|
| `Null` | `ShaderTargetApi::Null` |
| `DirectX11_SM5` | `ShaderTargetApi::DirectX11` |
| `DirectX12_SM6` | `ShaderTargetApi::DirectX12` |
| `Vulkan_SPIRV` | `ShaderTargetApi::Vulkan` |
| `OpenGL_GLSL450` | `ShaderTargetApi::OpenGL` |
| other | `ShaderTargetApi::Generic` |

### 4.4 `ResolveBinaryFormat`

```cpp
static ShaderBinaryFormat ResolveBinaryFormat(assets::ShaderTargetProfile profile) noexcept;
```

Returns the runtime-consumable binary format expected for the given target profile.

#### Mapping

| Target profile | Binary format |
|---|---|
| `Vulkan_SPIRV` | `ShaderBinaryFormat::Spirv` |
| `DirectX11_SM5` | `ShaderBinaryFormat::Dxbc` |
| `DirectX12_SM6` | `ShaderBinaryFormat::Dxil` |
| `OpenGL_GLSL450` | `ShaderBinaryFormat::GlslSource` |
| `Null` | `ShaderBinaryFormat::SourceText` |
| `Generic` | `ShaderBinaryFormat::SourceText` |

### 4.5 `ToString`

```cpp
static const char* ToString(assets::ShaderTargetProfile profile) noexcept;
```

Returns a stable debug string for logs.

### 4.6 `IsRuntimeConsumable`

```cpp
static bool IsRuntimeConsumable(const assets::CompiledShaderArtifact& shader) noexcept;
```

Returns `true` if the artifact contains at least one of:

- non-empty `bytecode`
- non-empty `sourceText`
- valid `contract`

### 4.7 `CompileForTarget`

```cpp
static bool CompileForTarget(const assets::ShaderAsset& asset,
                             assets::ShaderTargetProfile target,
                             assets::CompiledShaderArtifact& outCompiled,
                             std::string* outError = nullptr);
```

Compiles one shader asset for one target profile without extra variant defines.

#### Inputs

| Parameter | Meaning |
|---|---|
| `asset` | logical source asset |
| `target` | concrete compilation target |
| `outCompiled` | output artifact on success |
| `outError` | optional human-readable compiler error text |

#### Output contract

On success, `outCompiled` is filled with target-specific payload and contract metadata.

### 4.8 `VariantFlagsToDefines`

```cpp
static std::vector<std::string> VariantFlagsToDefines(ShaderVariantFlag flags) noexcept;
```

Maps engine variant bits to compiler preprocessor defines.

#### Mapping table

| Variant flag | Define emitted |
|---|---|
| `Skinned` | `KROM_SKINNING` |
| `VertexColor` | `KROM_VERTEX_COLOR` |
| `AlphaTest` | `KROM_ALPHA_TEST` |
| `NormalMap` | `KROM_NORMAL_MAP` |
| `Unlit` | `KROM_UNLIT` |
| `ShadowPass` | `KROM_SHADOW_PASS` |
| `Instanced` | `KROM_INSTANCED` |
| `BaseColorMap` | `KROM_BASECOLOR_MAP` |
| `MetallicMap` | `KROM_METALLIC_MAP` |
| `RoughnessMap` | `KROM_ROUGHNESS_MAP` |
| `OcclusionMap` | `KROM_OCCLUSION_MAP` |
| `EmissiveMap` | `KROM_EMISSIVE_MAP` |
| `OpacityMap` | `KROM_OPACITY_MAP` |
| `PBRMetalRough` | `KROM_PBR_METAL_ROUGH` |
| `DoubleSided` | `KROM_DOUBLE_SIDED` |
| `ORMMap` | `KROM_ORM_MAP` |
| `IBLMap` | `KROM_IBL` |

### 4.9 `CompileVariant`

```cpp
static bool CompileVariant(const assets::ShaderAsset& asset,
                           assets::ShaderTargetProfile target,
                           ShaderVariantFlag flags,
                           assets::CompiledShaderArtifact& outCompiled,
                           std::string* outError = nullptr);
```

Compiles one shader asset for one target profile plus the define set derived from `flags`.

---

## 5. Contract Types

Defined in `include/renderer/ShaderContract.hpp`.

### 5.1 `ShaderTargetApi`

```cpp
enum class ShaderTargetApi : uint8_t
{
    Generic = 0,
    Null,
    DirectX11,
    DirectX12,
    Vulkan,
    OpenGL,
};
```

Names the backend API namespace of a compiled shader contract.

### 5.2 `ShaderBinaryFormat`

```cpp
enum class ShaderBinaryFormat : uint8_t
{
    None = 0,
    SourceText,
    Spirv,
    Dxbc,
    Dxil,
    GlslSource,
};
```

Names the payload format carried by a compiled artifact.

### 5.3 `ShaderBindingClass`

```cpp
enum class ShaderBindingClass : uint8_t
{
    ConstantBuffer = 0,
    ShaderResource,
    Sampler,
    UnorderedAccess,
};
```

Classifies a binding declaration by resource category.

### 5.4 `ShaderBindingDeclaration`

```cpp
struct ShaderBindingDeclaration
{
    std::string        name;
    uint32_t           logicalSlot = 0u;
    uint32_t           apiBinding = 0u;
    uint32_t           space = 0u;
    ShaderBindingClass bindingClass = ShaderBindingClass::ConstantBuffer;
    uint32_t           stageMask = 0u;
};
```

Describes one shader-visible binding.

#### Field semantics

| Field | Meaning |
|---|---|
| `name` | shader symbol name |
| `logicalSlot` | engine slot number |
| `apiBinding` | backend-facing register or binding number |
| `space` | register space or descriptor set space |
| `bindingClass` | CBV/SRV/Sampler/UAV category |
| `stageMask` | stages that see the binding |

### 5.5 `ShaderInterfaceLayout`

```cpp
struct ShaderInterfaceLayout
{
    std::vector<ShaderBindingDeclaration> bindings;
    bool                                  usesEngineBindingModel = false;
    uint64_t                              layoutHash = 0ull;
};
```

Describes the interface extracted from the shader artifact.

### 5.6 `PipelineBindingContract`

```cpp
struct PipelineBindingContract
{
    PipelineBindingLayoutDesc   bindingLayout{};
    DescriptorRuntimeLayoutDesc runtimeLayout{};
    PipelineBindingSignatureDesc bindingSignature{};
    uint64_t                    bindingLayoutHash = 0ull;
    uint64_t                    runtimeLayoutHash = 0ull;
    uint64_t                    bindingSignatureHash = 0ull;
    uint64_t                    interfaceLayoutHash = 0ull;
    uint64_t                    bindingSignatureKey = 0ull;
    ComputeRuntimeDesc          computeRuntime{};
    uint64_t                    computeRuntimeHash = 0ull;

    [[nodiscard]] bool IsValid() const noexcept;
};
```

This is the pipeline-facing binding contract extracted or synthesized by the compiler layer.

### 5.7 `ShaderPipelineContract`

```cpp
struct ShaderPipelineContract
{
    ShaderTargetApi         api = ShaderTargetApi::Generic;
    ShaderBinaryFormat      binaryFormat = ShaderBinaryFormat::None;
    uint32_t                stageMask = 0u;
    ShaderInterfaceLayout   interfaceLayout{};
    PipelineBindingContract pipelineBinding{};
    uint64_t                pipelineStateKey = 0ull;
    uint64_t                contractHash = 0ull;

    [[nodiscard]] bool IsValid() const noexcept;
};
```

This is the main contract object stored inside `CompiledShaderArtifact`.

#### Validity rule

A contract is considered valid if any of the following is true:

- `binaryFormat != None`
- `interfaceLayout.usesEngineBindingModel == true`
- `pipelineBinding.IsValid() == true`

---

## 6. Binding Model

Defined in `include/renderer/ShaderBindingModel.hpp`.

This header contains the engine-level slot contract that shader code and runtime code must agree on.

### 6.1 Constant-buffer slots

```cpp
struct CBSlots
{
    static constexpr uint32_t PerFrame    = 0u;
    static constexpr uint32_t PerObject   = 1u;
    static constexpr uint32_t PerMaterial = 2u;
    static constexpr uint32_t PerPass     = 3u;
    static constexpr uint32_t COUNT       = 4u;
};
```

### 6.2 Texture slots

```cpp
struct TexSlots
{
    static constexpr uint32_t Albedo         = 0u;
    static constexpr uint32_t Normal         = 1u;
    static constexpr uint32_t ORM            = 2u;
    static constexpr uint32_t Emissive       = 3u;
    static constexpr uint32_t ShadowMap      = 4u;
    static constexpr uint32_t IBLIrradiance  = 5u;
    static constexpr uint32_t IBLPrefiltered = 6u;
    static constexpr uint32_t BRDFLUT        = 7u;
    static constexpr uint32_t PassSRV0       = 8u;
    static constexpr uint32_t PassSRV1       = 9u;
    static constexpr uint32_t PassSRV2       = 10u;
    static constexpr uint32_t HistoryBuffer  = 11u;
    static constexpr uint32_t BloomTexture   = 12u;
    static constexpr uint32_t COUNT          = 16u;
};
```

### 6.3 Sampler slots

```cpp
struct SamplerSlots
{
    static constexpr uint32_t LinearWrap  = 0u;
    static constexpr uint32_t LinearClamp = 1u;
    static constexpr uint32_t PointClamp  = 2u;
    static constexpr uint32_t ShadowPCF   = 3u;
    static constexpr uint32_t COUNT       = 4u;
};
```

### 6.4 UAV slots

```cpp
struct UAVSlots
{
    static constexpr uint32_t Output0 = 0u;
    static constexpr uint32_t Output1 = 1u;
    static constexpr uint32_t COUNT   = 8u;
};
```

### 6.5 Register mapping contract

```cpp
struct BindingRegisterRanges
{
    static constexpr uint32_t RegisterSpace      = 0u;
    static constexpr uint32_t ConstantBufferBase = 0u;
    static constexpr uint32_t ShaderResourceBase = 16u;
    static constexpr uint32_t SamplerBase        = 32u;
    static constexpr uint32_t UnorderedAccessBase= 48u;

    static constexpr uint32_t CB(uint32_t slot)  noexcept;
    static constexpr uint32_t SRV(uint32_t slot) noexcept;
    static constexpr uint32_t SMP(uint32_t slot) noexcept;
    static constexpr uint32_t UAV(uint32_t slot) noexcept;
};
```

#### Meaning

This maps engine logical slots to linear API binding indices.

| Category | Base register |
|---|---|
| constant buffers | `0` |
| shader resources | `16` |
| samplers | `32` |
| unordered access | `48` |

Shader source layouts must match these offsets.

### 6.6 Descriptor state records

`DescriptorBindingState` and `DescriptorMaterializationState` represent the runtime-side bound values and materialized resource states for CBs, textures and samplers.

These are internal runtime state containers, not authoring-facing asset types.

---

## 7. Variant Key API

Defined in `include/renderer/RendererTypes.hpp`.

### 7.1 `ShaderVariantFlag`

```cpp
enum class ShaderVariantFlag : uint32_t
{
    None             = 0u,
    Skinned          = 1u << 0,
    VertexColor      = 1u << 1,
    AlphaTest        = 1u << 2,
    NormalMap        = 1u << 3,
    Unlit            = 1u << 4,
    ShadowPass       = 1u << 5,
    Instanced        = 1u << 6,
    BaseColorMap     = 1u << 7,
    MetallicMap      = 1u << 8,
    RoughnessMap     = 1u << 9,
    OcclusionMap     = 1u << 10,
    EmissiveMap      = 1u << 11,
    OpacityMap       = 1u << 12,
    PBRMetalRough    = 1u << 13,
    DoubleSided      = 1u << 14,
    ORMMap           = 1u << 15,
    IBLMap           = 1u << 16,
};
```

This is the engine-neutral bitfield used by the runtime and compiler.

### 7.2 `ShaderPassType`

```cpp
enum class ShaderPassType : uint8_t
{
    Main   = 0,
    Shadow = 1,
    Depth  = 2,
    UI     = 3,
};
```

Used to distinguish pass-specific variants.

### 7.3 `ShaderVariantKey`

```cpp
struct ShaderVariantKey
{
    ShaderHandle      baseShader;
    ShaderPassType    pass  = ShaderPassType::Main;
    ShaderVariantFlag flags = ShaderVariantFlag::None;

    [[nodiscard]] ShaderVariantKey Normalized() const noexcept;
    [[nodiscard]] uint64_t Hash() const noexcept;

    bool operator==(const ShaderVariantKey& o) const noexcept;
};
```

#### Meaning

The key that uniquely identifies a compiled shader variant inside `ShaderVariantCache`.

---

## 8. Variant Cache API

Defined in `include/renderer/ShaderVariantCache.hpp`.

### 8.1 Class synopsis

```cpp
class ShaderVariantCache
{
public:
    using UploadFn = std::function<ShaderHandle(const assets::ShaderAsset&, const assets::CompiledShaderArtifact&)>;

    void SetUploadFunction(UploadFn fn);

    [[nodiscard]] ShaderHandle GetOrCreate(const assets::ShaderAsset& asset,
                                           assets::ShaderTargetProfile target,
                                           const ShaderVariantKey& key);

    void Clear();

    [[nodiscard]] size_t CachedCount() const noexcept;
    [[nodiscard]] uint32_t TotalVariants() const noexcept;
};
```

### 8.2 `SetUploadFunction`

Registers the callback used to upload a compiled artifact to GPU and return a `ShaderHandle`.

### 8.3 `GetOrCreate`

Creates or returns a cached GPU shader handle for a normalized variant key.

#### Behavior

1. normalize key
2. lookup in cache
3. compile variant if missing
4. upload compiled artifact through `UploadFn`
5. store resulting `ShaderHandle`
6. increment total variant counter

#### Failure cases

Returns `ShaderHandle::Invalid()` when:

- variant compilation fails
- upload callback is missing
- upload callback returns an invalid handle

### 8.4 `Clear`

Clears the cache and resets variant statistics.

### 8.5 Statistics

- `CachedCount()` returns currently cached entry count.
- `TotalVariants()` returns total successfully created variant count since last clear.

---

## 9. Shader Runtime State Types

Defined in `include/renderer/ShaderRuntime.hpp`.

### 9.1 `ShaderValidationIssue`

```cpp
struct ShaderValidationIssue
{
    enum class Severity : uint8_t { Warning, Error };
    Severity severity = Severity::Warning;
    std::string message;
};
```

Represents one material/shader validation message.

### 9.2 `ShaderAssetStatus`

```cpp
struct ShaderAssetStatus
{
    ShaderHandle assetHandle;
    ShaderHandle gpuHandle;
    ShaderStageMask stage = ShaderStageMask::None;
    assets::ShaderTargetProfile target = assets::ShaderTargetProfile::Generic;
    ShaderPipelineContract contract{};
    uint64_t compiledHash = 0ull;
    bool loaded = false;
    bool fromBytecode = false;
    bool fromCompiledArtifact = false;
};
```

Represents runtime preparation status for one shader asset.

### 9.3 `ResolvedMaterialBinding`

```cpp
struct ResolvedMaterialBinding
{
    enum class Kind : uint8_t { ConstantBuffer, Texture, Sampler };
    Kind kind = Kind::ConstantBuffer;
    std::string name;
    uint32_t slot = 0u;
    ShaderStageMask stages = ShaderStageMask::None;
    TextureHandle texture;
    uint32_t samplerIndex = 0u;
};
```

Represents one resolved binding entry used during runtime binding.

### 9.4 `MaterialGpuState`

```cpp
struct MaterialGpuState
{
    MaterialHandle material;
    PipelineHandle pipeline;
    ShaderHandle vertexShader;
    ShaderHandle fragmentShader;
    BufferHandle perMaterialCB;
    uint32_t perMaterialCBSize = 0u;
    uint64_t contentHash = 0u;
    uint64_t materialRevision = 0ull;
    uint64_t environmentRevision = 0ull;
    bool valid = false;
    std::vector<ResolvedMaterialBinding> bindings;
    std::vector<ShaderValidationIssue> issues;
};
```

Represents the GPU-resident runtime state for one material.

---

## 10. Shader Runtime API

Defined in `include/renderer/ShaderRuntime.hpp`.

### 10.1 Class synopsis

```cpp
class ShaderRuntime
{
public:
    bool Initialize(IDevice& device);
    void Shutdown();

    void SetAssetRegistry(assets::AssetRegistry* registry) noexcept;
    [[nodiscard]] assets::AssetRegistry* GetAssetRegistry() const noexcept;

    [[nodiscard]] bool CollectShaderRequests(const MaterialSystem& materials,
                                             std::vector<ShaderHandle>& outRequests) const;
    [[nodiscard]] bool CollectMaterialRequests(const MaterialSystem& materials,
                                               std::vector<MaterialHandle>& outRequests) const;

    [[nodiscard]] ShaderHandle PrepareShaderAsset(ShaderHandle shaderAssetHandle);
    [[nodiscard]] bool CommitShaderRequests(const std::vector<ShaderHandle>& requests);
    [[nodiscard]] bool PrepareAllShaderAssets();

    [[nodiscard]] bool PrepareMaterial(const MaterialSystem& materials, MaterialHandle material);
    [[nodiscard]] bool CommitMaterialRequests(const MaterialSystem& materials,
                                              const std::vector<MaterialHandle>& requests);
    [[nodiscard]] bool PrepareAllMaterials(const MaterialSystem& materials);

    [[nodiscard]] const MaterialGpuState* GetMaterialState(MaterialHandle material) const noexcept;
    [[nodiscard]] const ShaderAssetStatus* GetShaderStatus(ShaderHandle shaderAssetHandle) const noexcept;

    [[nodiscard]] ShaderHandle GetOrCreateVariant(ShaderHandle shaderAssetHandle,
                                                  ShaderPassType pass,
                                                  ShaderVariantFlag flags);
    [[nodiscard]] const ShaderVariantCache& GetVariantCache() const noexcept;

    [[nodiscard]] bool BindMaterial(ICommandList& cmd,
                                    const MaterialSystem& materials,
                                    MaterialHandle material,
                                    BufferHandle perFrameCB,
                                    BufferHandle perObjectCB,
                                    BufferHandle perPassCB = BufferHandle::Invalid(),
                                    RenderPassTag passOverride = RenderPassTag::Opaque);

    [[nodiscard]] bool BindMaterialWithRange(ICommandList& cmd,
                                             const MaterialSystem& materials,
                                             MaterialHandle material,
                                             BufferHandle   perFrameCB,
                                             BufferBinding  perObjectBinding,
                                             BufferBinding  perPassBinding = {},
                                             RenderPassTag  passOverride = RenderPassTag::Opaque);

    [[nodiscard]] bool ValidateMaterial(const MaterialSystem& materials,
                                        MaterialHandle material,
                                        std::vector<ShaderValidationIssue>& outIssues) const;

    void SetEnvironmentState(const EnvironmentRuntimeState& state) noexcept;
    [[nodiscard]] const EnvironmentRuntimeState& GetEnvironmentState() const noexcept;
    [[nodiscard]] bool HasIBL() const noexcept;

    [[nodiscard]] size_t PreparedShaderCount() const noexcept;
    [[nodiscard]] size_t PreparedMaterialCount() const noexcept;
    [[nodiscard]] bool IsRenderThread() const noexcept;
};
```

### 10.2 Lifecycle

#### `Initialize`

Initializes the runtime with a device and captures the render thread identity.

#### `Shutdown`

Releases runtime-owned GPU objects and clears caches.

### 10.3 Asset registry attachment

#### `SetAssetRegistry`

Attaches the CPU asset registry used to resolve `ShaderHandle` to `ShaderAsset`.

If no asset registry is attached, material preparation falls back to using handles already present in material descriptions.

### 10.4 Request collection

#### `CollectShaderRequests`

Scans the material system and emits a deduplicated list of shader asset handles referenced by material descriptions.

#### `CollectMaterialRequests`

Scans the material system and emits a deduplicated list of material handles that have valid material descriptions.

### 10.5 Shader preparation

#### `PrepareShaderAsset`

Ensures one shader asset is available as a GPU shader handle.

#### Source behavior summary

- must run on render thread
- validates device, asset registry, and handle
- returns existing prepared GPU handle if already cached
- resolves target profile from device
- compiles missing target artifact when needed
- uploads artifact or bytecode to the device
- stores `ShaderAssetStatus` in runtime cache

#### Return value

Returns the GPU shader handle or `ShaderHandle::Invalid()` on failure.

### 10.6 Material preparation

#### `PrepareMaterial`

Builds `MaterialGpuState` for a single material.

#### Source behavior summary

- must run on render thread
- validates material via `ValidateMaterial`
- computes variant flags from `MaterialSystem`
- adds `IBLMap` only when environment is active and all IBL textures are valid
- chooses pass type:
  - shadow pass -> `ShaderPassType::Shadow`
  - otherwise -> `ShaderPassType::Main`
- obtains shader variants through `GetOrCreateVariant`
- falls back to `PrepareShaderAsset` when variant compilation fails
- resolves texture/sampler bindings
- builds pipeline description and pipeline handle
- allocates and uploads per-material constant buffer when needed
- stores resulting `MaterialGpuState`

#### Return value

Returns `true` only if the resulting state is valid and the pipeline handle is valid.

### 10.7 Bulk preparation

#### `CommitShaderRequests`

Runs `PrepareShaderAsset` for all requested shader handles.

#### `PrepareAllShaderAssets`

Collects shader requests from the material system and prepares them.

#### `CommitMaterialRequests`

Runs `PrepareMaterial` for all requested material handles.

#### `PrepareAllMaterials`

Collects material requests and prepares them.

### 10.8 State queries

#### `GetMaterialState`

Returns prepared GPU state for one material or `nullptr` when not prepared.

#### `GetShaderStatus`

Returns cached runtime status for one shader asset or `nullptr` when not prepared.

### 10.9 Variant creation

#### `GetOrCreateVariant`

Builds a `ShaderVariantKey` from:

- base shader asset handle
- pass type
- variant flags

Then resolves device target profile and delegates to `ShaderVariantCache::GetOrCreate`.

### 10.10 Material binding

#### `BindMaterial`

Binds:

- pipeline
- per-frame constant buffer
- per-object constant buffer
- per-material constant buffer
- optional per-pass constant buffer
- resolved textures
- resolved samplers

Uses whole-buffer constant buffer binding.

#### `BindMaterialWithRange`

Same logical binding behavior, but uses `BufferBinding` ranges for per-object and per-pass constant buffers.

This is the preferred path for suballocated constant buffer arenas.

### 10.11 Validation

#### `ValidateMaterial`

Validates material compatibility and emits warnings/errors into `outIssues`.

The exact rule set lives in `src/renderer/ShaderRuntimeMaterials.cpp` and is runtime-facing rather than asset-authoring-facing.

### 10.12 Environment

#### `SetEnvironmentState`

Updates environment textures used by material binding and IBL variant activation.

#### `HasIBL`

Returns `true` when environment state is active.

### 10.13 Threading

Most mutating runtime operations are guarded by a render-thread check.

Observed methods with render-thread requirement:

- `PrepareShaderAsset`
- `PrepareMaterial`
- `CommitShaderRequests`
- `CommitMaterialRequests`
- `BindMaterial`
- `BindMaterialWithRange`

---

## 11. End-to-End Flow

### 11.1 Shader asset to GPU shader

1. `ShaderAsset` exists in `AssetRegistry`
2. runtime resolves target from `IDevice`
3. compiler creates or selects `CompiledShaderArtifact`
4. runtime uploads artifact to backend device
5. runtime stores `ShaderAssetStatus`

### 11.2 Material to bound draw state

1. material description references vertex and fragment shader assets
2. runtime computes material-driven `ShaderVariantFlag` bits
3. runtime adds environment-driven `IBLMap` when valid
4. runtime requests or creates shader variants
5. runtime builds pipeline
6. runtime resolves texture/sampler bindings
7. runtime builds per-material constant buffer
8. `BindMaterial` or `BindMaterialWithRange` emits command-list bindings

---

## 12. Minimal Usage Example

```cpp
using namespace engine;
using namespace engine::renderer;
using namespace engine::assets;

ShaderRuntime shaderRuntime;
shaderRuntime.Initialize(*device);
shaderRuntime.SetAssetRegistry(&assetRegistry);

ShaderHandle gpuVS = shaderRuntime.PrepareShaderAsset(vertexShaderAssetHandle);
ShaderHandle gpuPS = shaderRuntime.PrepareShaderAsset(fragmentShaderAssetHandle);

bool ok = shaderRuntime.PrepareMaterial(materialSystem, materialHandle);
if (!ok)
{
    // inspect validation issues or shader status here
}

shaderRuntime.BindMaterialWithRange(cmd,
                                    materialSystem,
                                    materialHandle,
                                    perFrameCB,
                                    perObjectBinding,
                                    perPassBinding,
                                    RenderPassTag::Opaque);
```

---

## 13. Non-Goals of This API

This API reference does **not** define:

- shader authoring style guide
- material authoring UI conventions
- file import pipeline rules outside the exposed shader asset types
- backend-specific compiler command lines
- shader source semantics beyond the engine binding model and variant defines

---

## 14. Practical Reading of the System

This shader system is not a single monolithic “custom shader language”.

It is a **multi-target shader runtime** built around:

- engine-owned shader assets
- engine-owned slot/binding contracts
- engine-owned variant flags
- backend-specific compilation targets
- runtime material-driven shader variant selection

So the real contract is not the filename. The real contract is:

- `ShaderAsset`
- `CompiledShaderArtifact`
- `ShaderPipelineContract`
- `ShaderVariantKey`
- `ShaderRuntime`
- `ShaderBindingModel`
