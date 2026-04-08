# Hello Window – Your First KROM Window

## Goal
Create a minimal window with an event loop.

## Code Example
```cpp 
#include "platform/GLFWPlatform.hpp"
// or Win32Platform.hpp

using namespace engine::platform;

int main()
{
    GLFWPlatform platform;
    if (!platform.Initialize())
        return -1;

    WindowDesc desc{};
    desc.title = "Hello KROM";
    desc.width = 1280;
    desc.height = 720;
    desc.resizable = true;

    IWindow* window = platform.CreateWindow(desc);
    if (!window)
        return -2;

    IInput* input = platform.GetInput();

    while (!window->ShouldClose())
    {
        platform.PumpEvents();

        if (input && input->IsKeyPressed(Key::Escape))
            window->RequestClose();

        // render logic
    }

    platform.Shutdown();
    return 0;
}
