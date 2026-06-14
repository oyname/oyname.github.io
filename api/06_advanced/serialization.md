# Serialization

`engine::serialization::SceneSerializer` · `engine::serialization::SceneDeserializer`

JSON-based scene serialization. Every living entity is captured regardless of which components it carries. Deserialization remaps entity IDs so that parent/child references survive a round-trip.

---

## Serializing a scene

```cpp
serialization::SceneSerializer serializer(world);
serializer.RegisterDefaultHandlers();

std::string json = serializer.SerializeToJson("Level01");
fs.WriteText("saves/level01.json", json);
```

`RegisterDefaultHandlers` registers serializers for all built-in components: `TransformComponent`, `WorldTransformComponent`, `NameComponent`, `MeshComponent`, `MaterialComponent`, `BoundsComponent`, `CameraComponent`, `LightComponent`, `ParentComponent`, `ChildrenComponent`, `ActiveComponent`.

---

## Custom component serializer

```cpp
serializer.RegisterSerializer<HealthComponent>(
    [](serialization::JsonWriter& w, const HealthComponent& h) {
        w.WriteFloat("current", h.current);
        w.WriteFloat("max",     h.max);
    });
```

The lambda receives a `JsonWriter` and the component value. It is called for every entity that carries the component.

---

## JsonWriter

```cpp
w.BeginObject("transform");
    w.WriteVec3("position", t.localPosition);
    w.WriteQuat("rotation", t.localRotation);
    w.WriteVec3("scale",    t.localScale);
w.EndObject();

w.BeginArray("tags");
    w.WriteString("", "visible");
    w.WriteString("", "static");
w.EndArray();
```

Available write methods: `WriteString`, `WriteFloat`, `WriteUint`, `WriteInt`, `WriteBool`, `WriteVec3`, `WriteQuat`.

---

## Deserializing a scene

```cpp
std::string json;
fs.ReadText("saves/level01.json", json);

serialization::SceneDeserializer deserializer(world);
deserializer.RegisterDefaultHandlers();

serialization::DeserializeResult result = deserializer.DeserializeFromJson(json);

if (!result.success)
    Debug::LogError("Deserialize failed: %s", result.error.c_str());

Debug::Log("Loaded %u entities, %u components", result.entitiesRead, result.componentsRead);
```

---

## Custom component deserializer

```cpp
deserializer.RegisterHandler<HealthComponent>(
    [](const serialization::JsonValue& v, ecs::World& world, EntityID id) {
        auto& h   = world.Add<HealthComponent>(id);
        h.current = v.Get("current") ? v.Get("current")->AsFloat() : 100.f;
        h.max     = v.Get("max")     ? v.Get("max")->AsFloat()     : 100.f;
    });
```

---

## JsonValue

The parser produces a `JsonValue` tree:

```cpp
const JsonValue* pos = v.Get("position");    // nullptr if key absent
float x = pos->AsVec3().x;

float val  = v.Get("speed")->AsFloat();
uint32_t n = v.Get("count")->AsUint();
bool flag  = v.Get("active")->AsBool();
const std::string& name = v.Get("name")->AsString();

// Arrays
for (size_t i = 0; i < v.arrayVal.size(); ++i)
    Process(v.At(i));
```

---

## DeserializeResult

```cpp
result.success           // false if JSON parsing or a handler threw
result.error             // error description
result.entitiesRead      // entities created
result.componentsRead    // components added
result.entitiesSkipped   // entities in JSON that could not be reconstructed
```

---

## Notes

- Serialization captures **all living entities** via `World::ForEachAlive`. Entities without a `NameComponent` are included.
- Entity IDs in the JSON are the original values at serialize time. Deserialization creates fresh IDs and builds a remap table so that `ParentComponent` references resolve correctly.
- The built-in JSON parser has no external dependencies. For large scenes or performance-critical pipelines, replace the serialization layer with a custom implementation backed by a faster JSON library.
