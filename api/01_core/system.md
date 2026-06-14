# System

KROM has no `ISystem` base class. Systems are free functions or lambdas that read and write components via `World::View`. This keeps systems decoupled from the engine lifecycle and straightforward to test in isolation.

---

## Basic system

```cpp
void MovementSystem(engine::ecs::World& world, float deltaTime) {
    world.View<TransformComponent, VelocityComponent>(
        [deltaTime](EntityID, TransformComponent& t, const VelocityComponent& v) {
            t.localPosition += v.velocity * deltaTime;
            t.dirty = true;
        });
}
```

Call it from your update loop:

```cpp
MovementSystem(world, dt);
```

---

## Read-only systems

Declare component parameters as `const` references for read-only access. The compiler enforces this — there is no separate concept of a "read system".

```cpp
void DebugPrintSystem(const engine::ecs::World& world) {
    world.View<NameComponent, TransformComponent>(
        [](EntityID, const NameComponent& name, const TransformComponent& t) {
            Debug::Log("%s @ (%.2f, %.2f, %.2f)",
                name.name.c_str(),
                t.localPosition.x, t.localPosition.y, t.localPosition.z);
        });
}
```

---

## Read phase for parallel access

When systems run on background threads or during the render extraction pass, wrap the iteration in a read phase. This prevents concurrent structural mutations (`Add`, `Remove`, `DestroyEntity`) from occurring while the view is active.

```cpp
void ExtractRenderData(const engine::ecs::World& world, RenderQueue& queue) {
    auto scope = world.ReadPhaseScope();

    world.View<TransformComponent, MeshComponent, MaterialComponent>(
        [&queue](EntityID id,
                 const TransformComponent& t,
                 const MeshComponent& m,
                 const MaterialComponent& mat) {
            queue.Push({ id, m.mesh, mat.material, t.worldMatrix });
        });
}
```

`ReadPhaseScope` is RAII — the phase is released when the scope object goes out of scope.

---

## Built-in systems

These systems are provided by Core and should be called once per frame in order.

| System | Header | Purpose |
|---|---|---|
| `TransformSystem::Update` | `scene/TransformSystem.hpp` | Propagates dirty flags, computes `WorldTransformComponent` |
| `BoundsSystem::Update` | `scene/BoundsSystem.hpp` | Updates world-space AABB from `WorldTransformComponent` |

`TransformSystem` and `BoundsSystem` are **instances**, not static utilities. Keep one of
each (they cache internal state between frames) and call `Update` on them:

```cpp
// Persisted instances (e.g. members of your app)
TransformSystem transforms;
BoundsSystem    bounds;

// Typical frame update order
transforms.Update(world);
bounds.Update(world);

// then: render extraction, physics, gameplay systems
```

---

## Ordering and dependencies

Systems have no built-in dependency graph. Order is determined by the call sequence in your update loop. If system B reads data that system A writes, call A before B.

For deferred structural mutations (adding or removing components during iteration), use `EntityCommandBuffer` to batch changes and apply them after the view completes.

```cpp
EntityCommandBuffer cmds;

world.View<HealthComponent>(
    [&cmds](EntityID id, const HealthComponent& hp) {
        if (hp.current <= 0.f)
            cmds.DestroyEntity(id);
    });

cmds.Commit(world);  // safe: outside the view
```
