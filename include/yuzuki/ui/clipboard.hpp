#pragma once
#include <yuzuki/core/types.hpp>

namespace yzk {

// Framework-level clipboard access: read/write plain Unicode text. This is the
// promoted, public version of the clipboard helpers TextBox used to keep
// private; every control that needs copy/paste (TextBox, ListView, ...) and any
// app code shares this single entry point instead of reimplementing Win32.
namespace clipboard {

// Returns true if the clipboard holds CF_UNICODETEXT. Safe to call anytime.
bool has_text();

// Copies `text` (UTF-8) to the system clipboard, replacing its current content.
// Returns false if the clipboard is owned by another process (busy) or the
// call failed for any other reason.
bool set_text(const String& text);

// Reads UTF-8 text from the system clipboard. Returns an empty string when no
// text is available. TextBox::paste_from_clipboard semantics apply: the caller
// decides how to handle empty results (typically "nothing to paste").
String get_text();

}  // namespace clipboard

}  // namespace yzk