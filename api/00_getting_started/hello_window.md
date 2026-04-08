# Hello Window – Your First KROM Window

## Goal
Create a minimal window with an event loop.

## Code Example
```cpp
#include <krom/platform/window.h>
#include <krom/engine.h>

int main() {
    // Initialize engine
    krom::Engine engine;
    
    // Create window
    auto window = krom::platform::Window::create({
        .title = "Hello KROM",
        .width = 1280,
        .height = 720,
        .vsync = true
    });
    
    // Main loop
    while (window->is_open()) {
        window->poll_events([](const krom::Event& e) {
            if (e.type == krom::Event::KeyPressed && 
                e.key == krom::Key::Escape) {
                window->close();
            }
        });
        
        // Insert render logic here
        engine.frame();
    }
    
    return 0;
}
