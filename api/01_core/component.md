# Component

Components are plain data structs. There is no engine base class to inherit from — any struct registered with `RegisterComponent<T>` can be added to an entity.

---

## Custom components

```cpp
struct HealthComponent {
    float current = 100.f;
    float max     = 100.f;
};

// Register into a ComponentMetaRegistry before constructing the World
engine::ecs::RegisterComponent<HealthComponent>(registry, "HealthComponent");
```

Registration is **per `ComponentMetaRegistry` instance**, not global. The same registry
is passed to the `World` constructor. The string name is used for serialization and
diagnostics.

> Components do not need to be trivially copyable. Archetype migration moves component
> data via the type's **move constructor** (then destroys the source), so components may
> hold `std::string`, `std::vector`, and other non-trivial members — the built-in
> `MeshComponent` and `MaterialComponent` do. The only hard requirement is that the type
> is move-constructible.

---

## Built-in components

There is no single `RegisterAllComponents()`. Registration is **per module**: call
`RegisterCoreComponents(registry)` for the core set below, plus the per-AddOn helpers for
the features you use, e.g. `RegisterMeshRendererComponents(registry)`,
`RegisterCameraComponents(registry)`, `RegisterLightingComponents(registry)`.

### Transform

| Component | Purpose |
|---|---|
| `TransformComponent` | Local position (`Vec3`), rotation (`Quat`), scale (`Vec3`) + dirty flag |
| `WorldTransformComponent` | Computed world matrix and its inverse — written by `TransformSystem` |
| `ParentComponent` | Parent entity for scene hierarchy |
| `ChildrenComponent` | List of child entities |

```cpp
auto& t = world.Add<TransformComponent>(entity);
t.localPosition = { 0.f, 1.f, 0.f };
t.SetEulerDeg(0.f, 90.f, 0.f);  // sets localRotation + marks dirty
```

### Rendering

| Component | Purpose |
|---|---|
| `MeshComponent` | Mesh handle, shadow casting flags, layer mask |
| `MaterialComponent` | Material handle + submesh index |
| `BoundsComponent` | AABB in local and world space — written by `BoundsSystem` |

### Camera

| Component | Purpose |
|---|---|
| `CameraComponent` | Projection type, FOV, near/far planes, aspect ratio, render layer |

```cpp
auto& cam = world.Add<CameraComponent>(entity);
cam.fovYDeg      = 60.f;
cam.nearPlane    = 0.1f;
cam.farPlane     = 1000.f;
cam.isMainCamera = true;
```

### Lighting

| Component | Purpose |
|---|---|
| `LightComponent` | Type (`Directional` / `Point` / `Spot`), color, intensity, range, shadow flag |

```cpp
auto& light = world.Add<LightComponent>(entity);
light.type      = LightType::Directional;
light.color     = { 1.f, 0.95f, 0.8f };
light.intensity = 3.f;
```

### Misc

| Component | Purpose |
|---|---|
| `NameComponent` | Debug / editor name string |
| `ActiveComponent` | Enable / disable an entity without destroying it |
| `ParticleEmitterComponent` | Emission rate, lifetime, velocity, size, color over lifetime |

---

## Constraints

- Components must be **move-constructible** — the archetype system relocates component data via the type's move constructor during migrations (not `memcpy`). Non-trivial members like `std::string` / `std::vector` are fine.
- Components should not hold raw owning pointers. Use handles (`MeshHandle`, `MaterialHandle`, etc.) or indices instead.
- A given component type can appear **at most once** per entity. For multiple instances, use child entities.
