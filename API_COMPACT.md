# OYNAME ENGINE API OVERVIEW

This document provides a high-level overview of the OYNAME Engine API.

It explains the actual engine path, the main runtime layers, and the minimal concepts required to get a scene on screen.

This document is intentionally architectural and usage-oriented.
For exact types, methods, fields, and function behavior, see the API Reference.

---

## 1. Purpose

The engine can be used in two ways:

1. **Direct engine path**
   - explicit access to window creation
   - DX11 device/context creation
   - render backend
   - ECS renderer
   - ECS registry
   - resource creation

2. **Simplified path through `gidx.h`**
   - wraps common setup steps
   - intended for quick demos and easier onboarding

This overview focuses on the **direct engine path**, because that is the actual engine structure.

---

## 2. Main Runtime Layers

The engine is split into several layers.

### `GIDXEngine`
Top-level runtime loop.
Responsible for:
- frame execution
- delta time
- event processing
- coordinating window and renderer

### `IGDXWindow` / `GDXWin32Window`
Platform window layer.
Responsible for:
- creating the OS window
- polling messages/events
- exposing native handles for graphics setup

### DX11 Context Layer
Responsible for:
- enumerating adapters
- selecting a GPU
- creating the DX11 device/swap chain context

### `IGDXRenderBackend` / `GDXDX11RenderBackend`
Graphics backend layer.
Responsible for wrapping the graphics context into an engine backend.

### `GDXECSRenderer`
Main renderer/runtime API.
Responsible for:
- renderer lifecycle
- ECS access
- shader, mesh, material, and texture creation
- frame rendering
- render targets and post-processing

### `Registry`
Central ECS storage.
Responsible for:
- entities
- components
- iteration over component sets

### Resource Stores
Handle-based storage for:
- meshes
- materials
- shaders
- textures
- render targets
- post-process resources

---

## 3. Typical Startup Path

Typical direct usage path:

1. create `GDXEventQueue`
2. create `WindowDesc`
3. create `GDXWin32Window`
4. enumerate DX11 adapters
5. create DX11 context with `GDXWin32DX11ContextFactory`
6. create `GDXDX11RenderBackend`
7. create `GDXECSRenderer`
8. create `GIDXEngine`
9. call `Initialize()`
10. create camera, mesh/material, and renderable entities
11. run with `Run()` or `Step()`

---

## 4. Runtime Flow

At runtime, the engine typically does the following each frame:

1. reset per-frame input state
2. poll window events
3. process queued events
4. update timing
5. begin render frame
6. execute update/tick logic
7. end frame and present

`Run()` executes this continuously.
`Step()` executes exactly one frame.

---

## 5. Events and Input

The platform window writes events into the event queue.
The engine consumes them once per frame.

Important built-in behavior:
- `Escape` is already handled by the engine
- pressing ESC triggers application shutdown behavior

Input is exposed as frame-based keyboard state:
- key held
- key pressed this frame
- key released this frame

---

## 6. Rendering Model

The engine is not built around immediate-mode draw calls.
Visible objects are created through the normal engine resource and ECS flow:

- create mesh data
- upload mesh resource
- create material resource
- create entity
- attach renderable and visibility-related components

This means even a first triangle is a normal mesh/material/entity case.

---

## 7. ECS Model

The engine uses ECS for scene representation.

Typical scene objects are entities with components such as:
- transform
- world transform
- renderable state
- visibility state
- bounds
- camera
- light

Rendering is driven by ECS data plus handle-based resource references.

Resources are not stored directly in components.
Components store typed handles to resources managed by the renderer.

---

## 8. Minimal Requirements for a Visible Scene

For a visible renderable object, you typically need:

- `TransformComponent`
- `WorldTransformComponent`
- `RenderableComponent`
- `VisibilityComponent`
- `RenderBoundsComponent`

For the active main camera, you typically need:

- `TransformComponent`
- `WorldTransformComponent`
- `CameraComponent`
- `ActiveCameraTag`

If this baseline is incomplete, nothing will render.

---

## 9. Minimal Initialization Example

Minimal direct initialization flow:

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

After initialization, typical next steps are:
- get the `Registry`
- create a camera entity
- build and upload a mesh
- create a material
- create a renderable entity

---

## 10. First Triangle: Conceptual Path

A first triangle is not a special API path.

It follows the normal engine model:

1. initialize engine and renderer
2. create an active camera
3. create CPU mesh data for one triangle
4. upload the mesh
5. create a material
6. create a renderable entity using mesh and material handles
7. run the frame loop

The important takeaway is:
the engine is ECS- and resource-driven, not immediate-mode driven.

---

## 11. When to Use `gidx.h`

`gidx.h` provides a simplified layer for:
- quick demos
- samples
- simple tools
- faster onboarding

Use the direct path when you want:
- explicit control
- engine-level understanding
- custom setup
- engine extension work

---

## 12. Summary

The direct engine path is:

1. create a window
2. create a DX11 context
3. create a backend
4. create the ECS renderer
5. create and initialize the engine
6. create ECS entities and resources
7. run the frame loop

The core design is:
- explicit initialization
- DX11-backed rendering
- ECS-based scene structure
- handle-based resources
- renderer-driven resource creation

