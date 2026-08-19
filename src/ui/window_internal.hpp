#pragma once
// Win32 helpers shared by the window implementation: modifier/mouse-button mapping
// and system cursor loading. For src/ui/window*.cpp only, not a public header.
#include <yuzuki/ui/window.hpp>

namespace yzk {

inline u8 map_mods() {
    u8 mods = KeyModifier_None;
    if (GetKeyState(VK_SHIFT) & 0x8000) mods |= KeyModifier_Shift;
    if (GetKeyState(VK_CONTROL) & 0x8000) mods |= KeyModifier_Control;
    if (GetKeyState(VK_MENU) & 0x8000) mods |= KeyModifier_Alt;
    if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000) mods |= KeyModifier_Win;
    return mods;
}

inline u8 map_mouse_buttons() {
    u8 buttons = MouseButton_None;
    if (GetKeyState(VK_LBUTTON) & 0x8000) buttons |= MouseButton_Left;
    if (GetKeyState(VK_RBUTTON) & 0x8000) buttons |= MouseButton_Right;
    if (GetKeyState(VK_MBUTTON) & 0x8000) buttons |= MouseButton_Middle;
    return buttons;
}

inline HCURSOR load_cursor(Cursor cursor) {
    switch (cursor) {
        case Cursor::Hand: return LoadCursorW(nullptr, IDC_HAND);
        case Cursor::IBeam: return LoadCursorW(nullptr, IDC_IBEAM);
        case Cursor::Cross: return LoadCursorW(nullptr, IDC_CROSS);
        case Cursor::SizeAll: return LoadCursorW(nullptr, IDC_SIZEALL);
        case Cursor::Arrow:
        default: return LoadCursorW(nullptr, IDC_ARROW);
    }
}

}  // namespace yzk