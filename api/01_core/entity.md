# Entity

`engine::EntityID`

A lightweight identifier for an object in the world. Entities have no data of their own — they are handles that index into the `World`. All state lives in components.

---

## Creation and destruction

```cpp
EntityID entity = world.CreateEntity();
world.DestroyEntity(entity);
```

After `DestroyEntity`, the `EntityID` becomes stale. Any subsequent call that takes it will either return `nullptr` (for `Get`) or `false` (for `IsAlive`, `Has`). Stale IDs never silently alias a newly created entity.

---

## Validity

```cpp
bool alive = world.IsAlive(entity);
```

`EntityID` encodes a generation counter alongside its index. An ID from a destroyed entity will not match any living entity even if the underlying slot is reused.

---

## Storage and passing

`EntityID` is a plain value type — copy, store, and pass by value freely.

```cpp
struct MyState {
    EntityID player;
    EntityID camera;
};
```

Long-lived IDs should be validated with `IsAlive` before use when the owning code does not control the entity's lifetime.

---

## NULL_ENTITY

```cpp
EntityID none = NULL_ENTITY;
none.IsValid();  // false
```

`NULL_ENTITY` is the zero-initialized sentinel. It will always fail `IsAlive`.

---

## Notes

- Entities are not typed. The same `EntityID` can hold any combination of components.
- Hierarchies are expressed via `ParentComponent` and `ChildrenComponent`, not through the `EntityID` itself.
- `EntityID` is safe to use as a map key — `std::hash` is specialized.
