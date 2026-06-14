# Transform

`engine::TransformComponent` · `engine::WorldTransformComponent`

Transform data is split into two components. `TransformComponent` holds the local values an author writes. `WorldTransformComponent` holds the computed world-space matrix and is written by the engine — never set it directly.

---

## TransformComponent

```cpp
struct TransformComponent {
    Vec3 localPosition { 0.f, 0.f, 0.f };
    Quat localRotation = Quat::Identity();
    Vec3 localScale    { 1.f, 1.f, 1.f };

    bool     dirty        = true;
    uint32_t localVersion = 1u;
    uint32_t worldVersion = 0u;

    void SetEulerDeg(float pitch, float yaw, float roll) noexcept;
};
```

Setting any field by hand requires marking the component dirty manually:

```cpp
auto* t = world.Get<TransformComponent>(entity);
t->localPosition = { 1.f, 0.f, 0.f };
t->dirty = true;
```

`SetEulerDeg` marks dirty automatically:

```cpp
t->SetEulerDeg(0.f, 90.f, 0.f);
```

---

## WorldTransformComponent

```cpp
struct WorldTransformComponent {
    Vec3 position { 0.f, 0.f, 0.f };
    Quat rotation = Quat::Identity();
    Vec3 scale    { 1.f, 1.f, 1.f };
    Mat4 matrix   = Mat4::Identity();
    Mat4 inverse  = Mat4::Identity();
};
```

All fields are updated by `TransformSystem::Update()`. Read them during render extraction or physics — do not write to them in gameplay code.

```cpp
const auto* world = ecs.Get<WorldTransformComponent>(entity);
Vec3 worldPos = world->matrix.TranslationColumn();
```

---

## TransformSystem

`engine::TransformSystem` — `scene/TransformSystem.hpp`

Propagates all dirty transforms in a single breadth-first forward pass. Parent entities are always processed before their children. Unmodified subtrees are skipped.

```cpp
TransformSystem transforms;
transforms.Update(world);
```

The system caches the sorted entity list and rebuilds it only when `world.StructureVersion()` changes.

```cpp
transforms.SortedEntityCount()  // entities in the propagation list
transforms.UpdateCount()        // entities processed in the last Update()
transforms.Invalidate()         // force rebuild on next Update()
```

---

## Using Scene for transform mutations

`engine::Scene` wraps `World` and exposes convenience helpers that handle the dirty flag automatically:

```cpp
Scene scene(world);
EntityID e = scene.CreateEntity("Cube");

scene.SetLocalPosition(e, { 0.f, 1.f, 0.f });
scene.SetLocalRotation(e, Quat::FromEulerDeg(0.f, 45.f, 0.f));
scene.SetLocalScale   (e, { 2.f, 2.f, 2.f });

scene.PropagateTransforms();  // calls TransformSystem::Update internally
```

---

## Update order

`TransformSystem::Update()` must be called before any system that reads `WorldTransformComponent`, including `BoundsSystem`, the render extraction pass, and physics queries.

```cpp
// Correct frame order
transforms.Update(world);
bounds.Update(world);            // or bounds.Update(world, jobSystem)
ExtractRenderData(world, renderQueue);
```
