# KROM ENGINE API OVERVIEW

This document provides a high-level overview of how to use the KROM ENGINE API.

It focuses on the practical engine path, the main objects you interact with, and the minimal steps required to get a scene on screen.

This document is intentionally usage-oriented.  
For architectural structure, see the Architecture document.  
For exact types, methods, fields, and function behavior, see the API Reference.

## Practical Usage Flow

The following diagram shows the practical user-facing path from initialization to the per-frame loop.

![KROM Engine Practical Usage Flow](krom_usage_flow.svg)

The short version is:

1. initialize the runtime
2. create the ECS world and scene
3. set up materials and the active view
4. create camera and scene entities
5. update ECS state and call `Tick(...)` every frame

---

## 1. Purpose

This document answers one practical question:

**How do you use the engine to create a window, set up rendering, build a scene, and run frames?**

KROM can be approached in two ways:

1. **Direct engine path**
   - explicit setup of platform, window and renderer
   - direct access to ECS world, scene data and rendering flow
   - intended for real engine usage, custom setup and advanced work

2. **Simplified entry path**
   - wraps common setup steps
   - reduces boilerplate
   - intended for quick demos and onboarding

This overview focuses on the **direct engine path**, because that is the path you use when working with the engine itself.

---

## 2. The Main Objects You Use

When using the engine directly, you mainly interact with a small set of runtime objects.

### `PlatformRenderLoop`

This is the outer runtime entry point.

You use it to:

- initialize the runtime
- connect the platform and window
- run one frame at a time
- shut the runtime down cleanly

### `ecs::World`

This is the core scene data container.

You use it to:

- create entities
- add components
- query scene data
- store the actual world state

### `Scene`

This is the higher-level scene helper built on top of the ECS world.

You use it to:

- create named entities
- build hierarchies
- manage parent-child relationships
- propagate transforms

### `MaterialSystem`

This manages material-side data used by rendering.

You use it to:

- create or register material state
- provide material data used by renderable entities

### `RenderView`

This describes the active view used for rendering.

You use it to:

- define which camera or view settings are active
- control what the renderer should render from

---

## 3. Basic Usage Flow

The direct usage flow is simple in principle:

1. create the platform and window setup
2. initialize `PlatformRenderLoop`
3. create the ECS world
4. create scene entities
5. create or assign materials
6. configure the active render view
7. call `Tick(...)` every frame
8. shut the runtime down when finished

That is the practical engine path.

---

## 4. Typical Startup Path

A typical startup sequence looks like this:

1. define a window description
2. initialize the runtime loop
3. create `ecs::World`
4. create `Scene`
5. create `MaterialSystem`
6. configure `RenderView`
7. create camera and scene entities
8. start the application loop
9. call `Tick(...)` every frame

In short:

- initialize runtime first
- build scene second
- run frames last

---

## 5. Minimal Initialization Example

A minimal direct initialization flow looks like this:

```cpp
platform::WindowDesc windowDesc{};
PlatformRenderLoop loop;

loop.Initialize(backendType, platform, windowDesc);

ecs::World world;
Scene scene(world);
MaterialSystem materials;

RenderView view{};
// configure camera or active view state here

while (running)
{
    loop.Tick(world, materials, view, timing);
}

loop.Shutdown();
```

This is the basic pattern you will follow in most applications using the direct path.

---

## 6. Creating Scene Content

Once the runtime is initialized, you create scene content through the ECS world and scene helpers.

Typical steps are:

1. create an entity
2. add transform data
3. add render-related components
4. assign mesh and material references
5. ensure the object is visible to the active view

In practice, the renderer works from scene state. You do not submit loose draw calls manually. You create entities and resources, and the renderer builds the frame from that data.

---

## 7. Creating a Camera

To render anything visible, you need an active camera or valid render view setup.

Typical camera setup means:

- create a camera entity
- add transform data
- add camera-related state
- make that camera the active view source

If no valid active view exists, the renderer has no meaningful point of view and the scene will not render as expected.

---

## 8. Creating a Visible Object

A visible object is usually created through normal scene and resource setup.

Typical requirements are:

- a valid transform
- renderable state
- mesh reference
- material reference
- visibility-related state
- valid bounds if required by visibility processing

This is important:

A first visible object is **not** a special case in the API. It follows the same engine model as every other rendered object.

That means the normal workflow is:

1. create entity
2. add required components
3. assign mesh/material data
4. run the frame

---

## 9. What Happens Per Frame

From the user side, one frame usually means one call to:

```cpp
loop.Tick(world, materials, view, timing);
```

That single call drives the full renderer frame for the current world state.

From a practical usage perspective, the frame loop normally looks like this:

1. update your game or application state
2. write changes into ECS components
3. update camera/view state
4. call `Tick(...)`
5. repeat

So the normal user-facing model is:

- update scene state
- let the renderer consume it
- render one frame

---

## 10. The Practical Mental Model

The simplest correct way to think about the engine is this:

- `ecs::World` stores the scene
- `Scene` helps build and organize that scene
- `MaterialSystem` provides material-side data
- `RenderView` defines how the scene should be viewed
- `PlatformRenderLoop` renders the current frame

That is the core usage model.

You are not expected to build a frame manually. You are expected to build scene state and then run the renderer.

---

## 11. Minimal Requirements for Something Visible

To get a visible result on screen, you generally need:

### A working runtime

The platform loop and renderer must initialize successfully.

### A valid active view

The renderer needs a valid camera or view configuration.

### A renderable scene object

At least one entity must contain the data required to be treated as visible render content.

If one of those three pieces is missing, you will usually get an empty frame.

---

## 12. First Visible Scene: Practical Path

A first visible scene usually follows this path:

1. initialize the runtime
2. create the ECS world
3. create the scene helper
4. create a camera
5. create one renderable object
6. assign its mesh and material
7. configure the active render view
8. run the frame loop

That is the basic path from engine startup to first image.

---

## 13. When to Use the Direct Path

Use the direct path when you want:

- explicit control over setup
- direct access to engine objects
- custom scene creation
- real engine integration
- full control over the runtime loop

Use the simplified path when you want:

- less setup code
- quick tests
- samples
- easier onboarding

For actual engine work, the direct path is the one that matters.

---

## 14. Summary

The practical usage path is:

1. initialize `PlatformRenderLoop`
2. create `ecs::World`
3. create `Scene`
4. create `MaterialSystem`
5. configure `RenderView`
6. create camera and scene entities
7. call `Tick(...)` every frame
8. shut down cleanly when done

The important point is simple:

**You use KROM by building scene state and running the frame loop.**

You do not work in an immediate-mode rendering style.  
You create and update world data, and the engine renders that world for you.
