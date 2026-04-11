# Input

`engine::platform::IInput`

Instance-based input abstraction. All state is owned by the `IInput` object — there is no global input state. `BeginFrame()` must be called once at the start of each frame to advance edge-triggered state (`KeyHit`, `KeyReleased`, mouse button hit/released).

---

## Accessing input

```cpp
platform::IInput* input = platform.GetInput();
```

The platform owns the `IInput` lifetime. Do not delete the returned pointer.

---

## Frame update

```cpp
while (!window->ShouldClose()) {
    platform.PumpEvents();   // feeds OS events into IInput
    input->BeginFrame();     // advances edge-triggered state

    // query input here
}
```

`PumpEvents` delivers raw OS events to the input backend. `BeginFrame` flips the edge-trigger buffers so that `KeyHit` and `KeyReleased` fire for exactly one frame per event.

---

## Keyboard

```cpp
// Held down
bool running = input->KeyDown(Key::LeftShift);

// Pressed this frame (rising edge)
if (input->KeyHit(Key::Space))
    Jump();

// Released this frame (falling edge)
if (input->KeyReleased(Key::E))
    CloseInventory();
```

Common keys: `Key::A`–`Key::Z`, `Key::Num0`–`Key::Num9`, `Key::F1`–`Key::F12`, `Key::Escape`, `Key::Space`, `Key::Enter`, `Key::Left`, `Key::Right`, `Key::Up`, `Key::Down`.

---

## Mouse

```cpp
// Button state
bool firing = input->MouseButtonDown(MouseButton::Left);
if (input->MouseButtonHit(MouseButton::Right))
    OpenContextMenu();

// Position and delta
int32_t x  = input->MouseX();
int32_t y  = input->MouseY();
int32_t dx = input->MouseDeltaX();
int32_t dy = input->MouseDeltaY();

// Scroll
float scroll = input->MouseScrollDelta();
```

---

## Cursor mode

```cpp
input->SetCursorMode(/*captured=*/true, /*hidden=*/true);
```

Captured mode locks the cursor to the window center and provides raw deltas — useful for first-person camera control.

---

## Event polling

For cases that need every individual key event (text input, replay recording):

```cpp
platform::InputEvent ev{};
while (input->PollEvent(ev)) {
    if (ev.type == platform::InputEventType::Key) {
        // ev.key.key, ev.key.pressed, ev.key.repeat
    }
}
```

---

## Key reference

```cpp
enum class Key : uint16_t {
    Escape, Space, Enter, Tab, Backspace,
    LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,
    Left, Right, Up, Down,
    A–Z,  Num0–Num9,  F1–F12,
    Plus, Minus, Equals,
    // ...
};

enum class MouseButton : uint8_t {
    Left, Right, Middle, X1, X2
};
```
