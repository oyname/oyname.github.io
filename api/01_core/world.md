# Core API

The Core API provides the foundational systems of KROM Engine. It is backend-agnostic and has no dependency on any rendering, platform, or addon layer.

All Core API types live in the `engine` or `engine::ecs` namespace. Call `RegisterAllComponents()` once before any `World` usage to make the built-in components available.

```cpp
RegisterAllComponents();
engine::ecs::World world;
```

---

- [World](world.md)
- [Entity](entity.md)
- [Component](component.md)
- [System](system.md)
