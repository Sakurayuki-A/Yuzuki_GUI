#pragma once
#include <yuzuki/core/types.hpp>

namespace yzk {

enum class EventType : u8 {
    None,
    MouseMove,
    MouseEnter,
    MouseLeave,
    MouseDown,
    MouseUp,
    Click,
    KeyDown,
    KeyUp,
    Character,
    Wheel,
    Resize,
    FocusGained,
    FocusLost,
    Timer,
    DragStart,
    DragMove,
    DragEnd,
    ImeCompose,   // composition (pre-edit) string updated; text may be empty (cancelled)
    ImeCommit,    // final committed string
};

enum MouseButton : u8 {
    MouseButton_None = 0,
    MouseButton_Left = 1,
    MouseButton_Middle = 2,
    MouseButton_Right = 4,
    MouseButton_X1 = 8,
    MouseButton_X2 = 16,
};

enum KeyModifier : u8 {
    KeyModifier_None = 0,
    KeyModifier_Shift = 1,
    KeyModifier_Control = 2,
    KeyModifier_Alt = 4,
    KeyModifier_Win = 8,
};

struct MouseData {
    f32 x = 0.0f;
    f32 y = 0.0f;
    u8 buttons = MouseButton_None;
    u8 mods = KeyModifier_None;
    i16 wheel_delta = 0;
};

struct KeyData {
    u32 code = 0;
    u16 chr = 0;
    u8 mods = KeyModifier_None;
    bool repeat = false;
};

struct ImeData {
    // Not owned by the event: points into Window-owned storage valid during dispatch.
    const wchar_t* text = nullptr;
    u32 length = 0;
    u32 cursor = 0;  // caret offset inside a composition string
};

union EventData {
    MouseData mouse;
    KeyData key;
    ImeData ime;
};

struct Event {
    EventType type = EventType::None;
    u32 time_ms = 0;
    EventData data{};
    bool consumed = false;
};

}  // namespace yzk