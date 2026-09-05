#pragma once
#include <yuzuki/core/types.hpp>
#include <yuzuki/core/event.hpp>

#include <functional>
#include <vector>

namespace yzk {

// A window-level keyboard accelerator: a key chord (VK code + modifier mask) bound
// to an action. Fired on the WindowKeyDown path, BEFORE the key is dispatched to
// the focused widget, so an app can bind e.g. Ctrl+S globally.
//
// Coordinating with focused input controls (the "don't hijack editing keys"
// rule): accelerators with no modifiers (plain F1..F12) or with Ctrl/Alt combos
// that an editing control already understands (Ctrl+Z/X/C/V/A, Ctrl+Left/Right
// word navigation, backspace, delete, arrows, home/end, enter) are SKIPPED while
// the focused widget is an input-style control (TextBox). This keeps typing
// intent intact; app-specific chords like Ctrl+S or Ctrl+Shift+T always win.
struct Accelerator {
    u32 vk = 0;       // Windows virtual-key code (see <windows.h> VK_*)
    u8 mods = KeyModifier_None;  // KeyModifier_Control | KeyModifier_Shift | KeyModifier_Alt | KeyModifier_Win
    std::function<void()> action;  // invoked when the chord fires
};

}  // namespace yzk