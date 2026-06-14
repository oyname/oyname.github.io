# Hierarchy

`engine::ParentComponent` · `engine::ChildrenComponent`

Scene hierarchies are expressed through two components. `ParentComponent` points upward to a parent entity. `ChildrenComponent` holds the list of direct children. Both are managed by `Scene` — add and remove hierarchy links through `Scene`, not by writing to the components directly.

---

## Setting a parent

```cpp
Scene scene(world);

EntityID parent = scene.CreateEntity("Vehicle");
EntityID wheel  = scene.CreateEntity("WheelFL");

scene.SetParent(wheel, parent);
```

After `SetParent`, `wheel` receives a `ParentComponent` pointing to `parent`, and `parent` receives or updates a `ChildrenComponent` containing `wheel`.

---

## Removing a parent link

```cpp
scene.DetachFromParent(wheel);
```

Both `ParentComponent` on the child and the entry in `ChildrenComponent` on the parent are removed. The child becomes a root entity.

---

## Reading hierarchy data

```cpp
const auto* p = world.Get<ParentComponent>(wheel);
if (p && world.IsAlive(p->parent)) {
    // wheel has a live parent
}

const auto* children = world.Get<ChildrenComponent>(parent);
if (children) {
    for (EntityID child : children->children) {
        // ...
    }
}
```

---

## Root entities

`Scene` tracks which entities have no parent:

```cpp
const std::vector<EntityID>& roots = scene.GetRoots();
```

Root entities are the starting points of the transform propagation pass. Every entity reachable from a root by following `ChildrenComponent` will have its `WorldTransformComponent` updated.

---

## Transform propagation and hierarchy

`TransformSystem` performs a breadth-first traversal. The sorted propagation list is rebuilt automatically when the hierarchy changes (detected via `world.StructureVersion()`).

Children always inherit the world matrix of their parent. If a parent's transform is not dirty, the subtree below it is skipped.

---

## Destroying a hierarchy

`Scene::DestroyEntity` destroys the entity and detaches it cleanly from any parent. It does not recursively destroy children — destroy each child explicitly if the entire subtree should be removed.

```cpp
// Destroy all children first, then the parent
for (EntityID child : scene.GetWorld().Get<ChildrenComponent>(parent)->children)
    scene.DestroyEntity(child);

scene.DestroyEntity(parent);
```

---

## Finding entities by name

```cpp
EntityID found = scene.FindByName("WheelFL");
if (world.IsAlive(found)) {
    // found it
}
```

`FindByName` scans all entities with a `NameComponent`. It returns `NULL_ENTITY` when no match is found.
