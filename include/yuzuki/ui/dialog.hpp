#pragma once
#include <yuzuki/ui/overlay.hpp>
#include <yuzuki/controls/button.hpp>
#include <yuzuki/controls/label.hpp>
#include <yuzuki/controls/stack_panel.hpp>

#include <functional>
#include <vector>

#undef MessageBox  // winuser.h may map MessageBox→MessageBoxW; keep our class name stable

namespace yzk {

class Window;

// ===== MessageBox (framework-rendered, non-blocking modal) =====
enum class MessageBoxKind : u8 {
    Info,        // [OK]
    Confirm,     // [OK] [Cancel]
    YesNo,       // [Yes] [No]
    YesNoCancel, // [Yes] [No] [Cancel]
};

// Result reported to the callback when the box is dismissed (any button, Esc, or backdrop).
enum class MessageBoxResult : u8 {
    None,    // dismissed via backdrop click / close / Esc on single-button box
    Ok,
    Cancel,
    Yes,
    No,
};

// Fully framework-rendered modal message box built on Overlay (dim mask + centered
// panel + intro/outro animations). Non-blocking: show() returns immediately; the
// callback fires once on dismissal. Escape maps to Cancel (or the only button, if
// the box has a single button); backdrop click dismisses as None.
class MessageBox {
public:
    MessageBox();
    ~MessageBox();

    MessageBox(const MessageBox&) = delete;
    MessageBox& operator=(const MessageBox&) = delete;

    void show(Window& win, const String& title, const String& message,
              MessageBoxKind kind,
              std::function<void(MessageBoxResult)> on_result = nullptr);
    void close();
    bool is_open() const;

    // Finish the dialog with an explicit result as if its button was pressed.
    // Escape/backdrop dismissals map to Cancel/None internally.
    void resolve(MessageBoxResult result);

    // Disable the dim/panel intro animation (synchronous show/close). Tests and
    // rapid re-shows use this; the default is animated for a polished feel.
    void set_animated(bool animated);

    struct Impl;  // pimpl (public so the render panel can reach it); not API

private:
    Impl* impl_;
};

// ===== Native file dialogs (IFileOpenDialog / IFileSaveDialog) =====
struct FileDialogFilter {
    String name;     // display name, e.g. "Text files"
    String pattern;  // wildcard, e.g. "*.txt" (may contain ';' separated patterns)
};

struct FileDialogOptions {
    std::vector<FileDialogFilter> filters;
    bool multi_select = false;      // open mode only
    String default_name;            // initial filename in save mode
};

// Modal native open/save dialogs owned by `owner` (may be null → dialog has no
// owner). Block until the user picks or cancels. UTF-8 paths are returned;
// empty vector / empty string mean "cancelled".
std::vector<String> open_file_dialog(Window* owner, const FileDialogOptions& opts);
String save_file_dialog(Window* owner, const FileDialogOptions& opts);

}  // namespace yzk