# World

`engine::ecs::World`

The central container for all entities and components. `World` manages entity lifecycles, component storage via archetype-based chunks, and provides the query interface for systems.

---

## Creating a world

```cpp
engine::ecs::World world;
```

`World` is non-copyable. One instance per simulation context is the expected usage.

---

## Entity lifecycle

```cpp
EntityID entity = world.CreateEntity();
bool     alive  = world.IsAlive(entity);
size_t   count  = world.EntityCount();

world.DestroyEntity(entity);
```

---

## Components

```cpp
// Add a component (constructs in-place)
auto& transform = world.Add<TransformComponent>(entity);
world.Add<MeshComponent>(entity, myMeshHandle);

// Read / write
TransformComponent* t = world.Get<TransformComponent>(entity);  // nullptr if absent
bool hasMesh          = world.Has<MeshComponent>(entity);

// Remove
world.Remove<MeshComponent>(entity);
```

> **Important:** `Add<T>()` migrates the entity to a new archetype. Any reference or pointer obtained before the call is invalidated. Always re-fetch via `Get<T>()` after structural mutations.
>
> ```cpp
> world.Add<TransformComponent>(entity);
> world.Add<MeshComponent>(entity);          // TransformComponent ref is now invalid
> auto* t = world.Get<TransformComponent>(entity);  // correct: re-fetch
> ```

---

## Querying

`View` iterates every entity that has all listed component types.

```cpp
world.View<TransformComponent, MeshComponent>(
    [](EntityID id, TransformComponent& t, MeshComponent& m) {
        // called once per matching entity
    });
```

Multiple component types are matched via archetype intersection — no per-entity branch is taken for non-matching entities.

Const overload for read-only access:

```cpp
world.View<TransformComponent>(
    [](EntityID id, const TransformComponent& t) { ... });
```

---

## Read phase

Structural mutations (`Add`, `Remove`, `DestroyEntity`) are not permitted while a read phase is active. Use `ReadPhaseScope()` to guard parallel or deferred read passes.

```cpp
{
    auto scope = world.ReadPhaseScope();
    world.View<TransformComponent>([](EntityID, const TransformComponent& t) { ... });
}
// mutations allowed again here
```

`ReadPhaseScope` is move-only and releases the phase on destruction.

---

## Diagnostics

```cpp
world.EntityCount()      // number of alive entities
world.ArchetypeCount()   // number of distinct component combinations
world.StructureVersion() // increments on every Add / Remove / Destroy
```

`StructureVersion` can be used to detect whether the world layout has changed between frames — useful for cache invalidation in systems that cache query results.
