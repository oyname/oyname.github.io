# Window

`engine::platform::IWindow` · `engine::platform::IPlatform`

Windows are created through a platform implementation. KROM provides `Win32Platform` for native Windows builds and `GLFWPlatform` for cross-platform use. The rest of the engine only sees `IWindow` and `IPlatform`.

---

## Creating a window

```cpp
#include "platform/Win32Platform.hpp"
// or: #include "platform/GLFWPlatform.hpp"

platform::Win32Platform platform;
if (!platform.Initialize())
    return -1;

platform::WindowDesc desc{};
desc.title  = "My App";
desc.width  = 1280u;
desc.height = 720u;
desc.resizable  = true;
desc.fullscreen = false;

platform::IWindow* window = platform.CreateWindow(desc);
```

---

## Event loop

```cpp
while (!window->ShouldClose()) {
    platform.PumpEvents();

    // use input, render, etc.
}

platform.Shutdown();
```

`PumpEvents` processes OS messages and feeds input events into `IInput`. It must be called once per frame before reading input state.

---

## WindowDesc fields

| Field | Default | Notes |
|---|---|---|
| `width` | `1280` | Client area width in pixels |
| `height` | `720` | Client area height |
| `title` | `"KROM Engine"` | Window title bar string |
| `resizable` | `true` | Allow resize by the user |
| `fullscreen` | `false` | Exclusive fullscreen |
| `openglContext` | `false` | Set `true` for OpenGL backends |
| `openglMajor` | `4` | OpenGL major version |
| `openglMinor` | `1` | OpenGL minor version |

---

## IWindow methods

```cpp
window->GetWidth()          // current client width
window->GetHeight()         // current client height
window->GetFramebufferWidth()   // framebuffer width (accounts for DPI)
window->GetFramebufferHeight()
window->GetDPIScale()       // e.g. 1.5 on a 150% display
window->GetNativeHandle()   // HWND on Win32, GLFWwindow* on GLFW
window->GetBackendName()    // "Win32Window", "GLFWWindow", ...

window->Resize(width, height)
window->SetTitle("New Title")
window->RequestClose()
window->IsOpen()
window->ShouldClose()
```

---

## Headless window

For tests and server builds that do not render to a display:

```cpp
auto headless = platform::CreateHeadlessWindow();
headless->Create(desc);
```

`HeadlessWindow` accepts the same `WindowDesc` and responds to `ShouldClose()` correctly but never opens a real OS window.

---

## Timing

```cpp
double seconds = platform.GetTimeSeconds();
```

Returns monotonic time in seconds since `Initialize()`. Useful for computing `deltaTime`:

```cpp
double prev = platform.GetTimeSeconds();
while (!window->ShouldClose()) {
    platform.PumpEvents();
    double now = platform.GetTimeSeconds();
    float dt   = static_cast<float>(now - prev);
    prev = now;
}
```
