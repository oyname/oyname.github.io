# GDX Rendering Architecture

> Screenshot-Look in echtem Markdown geht nicht sauber.  
> HTML-in-MD bricht je nach Renderer. Deshalb hier eine Version, die **überall lesbar** ist und **nicht auseinanderfällt**.

---

## Architektur-Übersicht

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│ Layer 0 — Public API (gidx.h)                                               │
│ Engine::Graphics() · CreateMesh(...) · Run(tickFn)                          │
│ Flat wrapper · keine ECS-Typen sichtbar · LPENTITY / LPMATERIAL / LPMESH    │
└──────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ Layer 1 — GDXECSRenderer                                                    │
│ Backend-Owner · Registry · ResourceStores · Frame lifecycle                 │
│ BeginFrame → EndFrame                                                       │
│ ShaderVariantCache · PostProcess management · FreeCamera                    │
└──────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ Layer 2 — ECS + Resources                                                   │
│ Registry · Systems · ResourceStores · Scheduler                             │
└──────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ Layer 3 — Frame Pipeline (RFG)                                              │
│ PipelineData · ViewPassData · FrameGraph · FrameDispatch · Post-process     │
└──────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ Layer 4 — IGDXRenderBackend                                                 │
│ Backend-neutrale Schnittstelle · keine DX-Typen · keine void*               │
└──────────────────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
┌───────────────────────────────┬───────────────────────┬──────────────────────┐
│ Layer 5 — DX11 backend        │ DX12 backend          │ OpenGL               │
│ primary implementation        │ stub                  │ stub                 │
└───────────────────────────────┴───────────────────────┴──────────────────────┘
                                      │
                                      ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│ Layer 6 — HLSL shaders                                                      │
│ PBR · Shadows · Forward+ · PostProcess                                      │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## Layer 0 — Public API (`gidx.h`)

**Zweck**  
Flacher OYNAME-Style-Wrapper. Die öffentliche API versteckt die ECS-Interna komplett.

**Öffentliche Oberfläche**
- `Engine::Graphics()`
- `CreateMesh(...)`
- `Run(tickFn)`

**Sicht des Nutzers**
- `LPENTITY`
- `LPMATERIAL`
- `LPMESH`

---

## Layer 1 — `GDXECSRenderer`

**Rolle**  
Das Herzstück des Systems.

**Besitzt**
- `unique_ptr<IGDXRenderBackend>`
- alle `ResourceStore`s
- die `Registry`
- den kompletten Frame-Lifecycle

**Frame-Einstiegspunkt**
- `BeginFrame() -> EndFrame()`

**Interne Bausteine**
- `GDXShaderVariantCache`  
  Cache-Key: **Pass × Submesh × Material**
- Post-Process-Pass-Management
- `FreeCamera`

---

## Layer 2 — ECS + Ressourcen

### Registry

- `ComponentPool<T>`
- generationsbasierte `EntityID`
  - 24-Bit Index
  - 8-Bit Generation

### Systems

| System | Aufgabe |
|---|---|
| `TransformSystem` | Transformdaten aktualisieren |
| `CameraSystem` | Kameradaten vorbereiten |
| `ViewCullingSystem` | sichtbare Objekte bestimmen |
| `RenderGatherSystem` | Render-Queues füllen |

### ResourceStores

| Typ | Beschreibung |
|---|---|
| `Handle<Tag>` | typisierte 32-Bit-Generation-Handles |
| `ResourceStore<T,Tag>` | typisierte Slot-Allocatoren |

**Store-Instanzen**
- Mesh
- Material
- Shader
- Texture
- RenderTarget
- PostProcess

### Scheduling

- `SystemScheduler` baut Bitmask-abhängige Batches
- `Execute(nullptr)` läuft seriell

---

## Layer 3 — Frame Pipeline (`namespace RFG`)

### `RFG::PipelineData`

Enthält:
- `mainView`
- `rttViews`

Jeder Eintrag ist ein `ViewPassData`.

### `ViewPassData`

Enthält:
- `ExecuteData`
  - eingefrorener Frame-Snapshot
  - getrennte Queues für:
    - opaque
    - alpha
    - shadow
- `VisibleSet`
- `ViewStats`

### `FrameGraph`

- geordnete Node-Liste
- jeder Node besitzt eine `executeFn`-Lambda

### `FrameDispatch`

Enthält alle Context-Structs mit **Frame-Lifetime**.

**Warum das wichtig ist**  
Das behebt das frühere Stack-Lifetime-Problem lokaler Variablen in `EndFrame()`.

### Post-Process

- Bloom
- ToneMap
- FXAA
- GTAO
- ping-pong surfaces

---

## Layer 4 — `IGDXRenderBackend`

**Ziel**  
Vollständig backend-neutrale Schnittstelle.

**Eigenschaften**
- keine DX-Typen
- keine `void*`

**Optionale Methoden**
Alle optionalen Methoden haben No-op-Defaults, zum Beispiel für:
- PostProcess
- RenderTarget
- IBL-Capabilities

---

## Layer 5 — DX11-Backend (primary)

`GDXDX11RenderBackend` ist die primäre Implementierung.

### GPU-Ownership

Liegt in `GDXDX11GpuRegistry` mit 6 `unordered_map`s:
- Texture
- RenderTarget
- Shader
- Mesh
- Material
- PostProcess

### `DX11MeshGpu`

Separate Vertex-Buffer-Streams pro Semantik:
- Position
- Normal
- UV
- Tangent
- Bone

### `DX11RenderTargetGpu`

Trägt:
- Color RTV + SRV
- Depth RTV + SRV
- Normal RTV + SRV

Das wird unter anderem für **GTAO** genutzt.

### Andere Backends

| Backend | Status |
|---|---|
| DX11 | primär |
| DX12 | Stub |
| OpenGL | Stub |

---

## Layer 6 — HLSL-Shaders (Runtime-Kompilierung)

### PBR-Shaders

`PixelShader.hlsl` mit:
- Cook-Torrance GGX
- `ddx/ddy` TBN
- CSM PCF 3×3
- IBL über `t17 / t18 / t19`

### Shadow-Shaders

Vier Varianten:
- plain
- AlphaTest
- Skinned
- SkinnedAlphaTest

### Forward+

`TileLightCullCS.hlsl`:
- 16×16 Tiles
- 256 Lichter
- Ausgabe nach `t20 / t21 / t22`

### PostProcess-Shaders

- Bloom
- ToneMap / ACES
- FXAA
- GTAO + Blur + Composite
- Edge / Normal / Depth Debug

---

## Zusatz: IBL

`GDXIBLBaker` arbeitet asynchron:

```text
HDR input
   │
   ├─► irradiance
   ├─► prefiltered environment
   └─► BRDF-LUT
```

---
