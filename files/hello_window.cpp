// hello_window.cpp
//
// KROM Engine — Hello Window
//
// Opens a resizable 1280x720 window and runs a minimal event loop.
// Press Escape to close.
//
// No renderer, no ECS, no assets — the simplest possible KROM entry point.
//
// Build (Visual Studio, x64, Windows):
//   Link against: engine_core, engine_platform_win32
//
// For cross-platform builds with GLFW replace Win32Platform with GLFWPlatform
// and link against engine_platform_glfw instead.

#include "platform/Win32Platform.hpp"
#include "platform/IWindow.hpp"
#include "platform/IInput.hpp"
#include "core/Debug.hpp"

using namespace engine::platform;

int main()
{
    Debug::MinLevel = engine::LogLevel::Info;

    win32::Win32Platform platform;
    if (!platform.Initialize())
    {
        Debug::LogError("hello_window: platform init failed");
        return -1;
    }

    WindowDesc desc{};
    desc.title     = "KROM - Hello Window";
    desc.width     = 1280u;
    desc.height    = 720u;
    desc.resizable = true;

    IWindow* window = platform.CreateWindow(desc);
    if (!window)
    {
        Debug::LogError("hello_window: window creation failed");
        platform.Shutdown();
        return -2;
    }

    IInput* input = platform.GetInput();

    Debug::Log("hello_window: running — press Escape to close");

    while (!window->ShouldClose())
    {
        platform.PumpEvents();

        if (input)
        {
            input->BeginFrame();

            if (input->KeyHit(Key::Escape))
                window->RequestClose();
        }
    }

    platform.Shutdown();
    return 0;
}
