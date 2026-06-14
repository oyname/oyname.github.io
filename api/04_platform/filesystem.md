# Filesystem

`engine::platform::IFilesystem`

Platform-neutral file access. Engine systems — the shader compiler, texture loader, scene serializer — call `IFilesystem` exclusively. No `fopen` or `std::ifstream` appears in engine code. This makes every file-dependent system testable with an in-memory replacement.

---

## Default implementation

`StdFilesystem` wraps the C++ standard library and is used by default when no custom filesystem is provided.

```cpp
#include "platform/StdFilesystem.hpp"

platform::StdFilesystem fs;
fs.SetAssetRoot("assets/");
```

---

## Reading files

```cpp
std::vector<uint8_t> binary;
if (fs.ReadFile("textures/rock.png", binary)) {
    // binary contains the file bytes
}

std::string source;
if (fs.ReadText("shaders/unlit.hlslvs", source)) {
    // source contains the text
}
```

---

## Writing files

```cpp
fs.WriteText("saves/checkpoint.json", jsonString);
fs.WriteFile("cache/shader.dxbc", bytecode.data(), bytecode.size());
```

---

## File queries

```cpp
bool exists = fs.FileExists("textures/rock.png");

platform::FileStats stats = fs.GetFileStats("textures/rock.png");
stats.exists                // bool
stats.sizeBytes             // file size
stats.lastModifiedTimestamp // platform timestamp (for hot-reload)
stats.isDirectory           // true if it's a folder
```

---

## Asset root and path resolution

```cpp
fs.SetAssetRoot("assets/");

// Resolves relative → absolute using the asset root
std::string abs = fs.ResolveAssetPath("textures/rock.png");
// → "assets/textures/rock.png" (or absolute path on disk)
```

---

## Directory operations

```cpp
fs.CreateDirectories("cache/shaders/dx11");

std::vector<std::string> files;
fs.ListFiles("textures/", "*.png", files);
```

---

## Injecting a custom filesystem

Pass an `IFilesystem*` into systems that accept one. This is the standard pattern for replacing real file I/O in tests:

```cpp
class NullFilesystem : public platform::IFilesystem {
    bool ReadFile(const char*, std::vector<uint8_t>& out) override {
        out = { /* test data */ };
        return true;
    }
    // implement other methods as no-ops or stubs
};

NullFilesystem nullFs;
assets::AssetPipeline pipeline(registry, device, &nullFs);
```

Systems that receive `nullptr` fall back to their own internal `StdFilesystem`.
