// Window: lifecycle / widget-tree mounting / focus state / timers.
// Rendering, input, borderless and the window procedure live in window_frame.cpp /
// window_input.cpp / window_borderless.cpp / window_wndproc.cpp
#include <yuzuki/ui/window.hpp>

#include <yuzuki/ui/application.hpp>
#include <yuzuki/ui/debug_overlay.hpp>
#include <yuzuki/ui/widget_inspector.hpp>
#include <yuzuki/core/encoding.hpp>
#include "ui/window_internal.hpp"
#include "render/d2d/d2d_backend.hpp"

#include <imm.h>
#include <shellapi.h>
#include <mmsystem.h>

#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#endif

namespace yzk {

namespace {

constexpr wchar_t kWindowClass[] = L"YuzukiUI.Window";

}  // namespace

Window::Window(const String& title, u32 width_dip, u32 height_dip)
    : title_(title),
      client_width_(width_dip),
      client_height_(height_dip) {
    bounds_ = RectF::make(0.0f, 0.0f, static_cast<f32>(width_dip), static_cast<f32>(height_dip));
    backend_ = new D2DBackend();
}

Window::~Window() {
    destroy();
    delete backend_;
}

bool Window::create(void* instance) {
    if (hwnd_) return true;
    closing_ = false;

    HINSTANCE hinst = static_cast<HINSTANCE>(instance);
    if (!hinst) hinst = static_cast<HINSTANCE>(GetModuleHandleW(nullptr));
    if (!hinst) return false;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = &Window::wnd_proc;
    wc.hInstance = hinst;
    wc.hCursor = load_cursor(Cursor::Arrow);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClass;
    if (RegisterClassExW(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    const DWORD style =
        borderless_ ? (WS_POPUP | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU)
                    : WS_OVERLAPPEDWINDOW;
    const DWORD ex_style = topmost_ ? WS_EX_TOPMOST : 0;
    HWND owner_hwnd = owner_ ? static_cast<HWND>(owner_->native_handle()) : nullptr;
    HWND hwnd = CreateWindowExW(
        ex_style, kWindowClass, utf::to_wide(title_).c_str(), style,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, owner_hwnd, nullptr, hinst, nullptr);
    if (!hwnd) return false;

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    hwnd_ = hwnd;
    DragAcceptFiles(hwnd, TRUE);

    const u32 dpi = GetDpiForWindow(hwnd);
    const f32 scale = static_cast<f32>(dpi) / 96.0f;
    const u32 client_px_w = static_cast<u32>(static_cast<f32>(client_width_) * scale + 0.5f);
    const u32 client_px_h = static_cast<u32>(static_cast<f32>(client_height_) * scale + 0.5f);

    RECT wr{0, 0, static_cast<LONG>(client_px_w), static_cast<LONG>(client_px_h)};
    if (!borderless_) AdjustWindowRectEx(&wr, style, FALSE, 0);
    SetWindowPos(hwnd, nullptr, 0, 0, wr.right - wr.left, wr.bottom - wr.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

    if (!backend_->create_target(hwnd, client_px_w, client_px_h, dpi)) {
        hwnd_ = nullptr;
        DestroyWindow(hwnd);
        return false;
    }

    const f32 real_scale = backend_->dpi_scale();
    client_width_ = static_cast<u32>(static_cast<f32>(client_px_w) / real_scale + 0.5f);
    client_height_ = static_cast<u32>(static_cast<f32>(client_px_h) / real_scale + 0.5f);
    bounds_ = RectF::make(0.0f, 0.0f, static_cast<f32>(client_width_), static_cast<f32>(client_height_));

    Application::instance().add_window(this);
    if (owner_) update_owner_state();
    invalidate_all();
    return true;
}

void Window::update_owner_state() {
    if (!hwnd_) return;
    HWND hwnd = static_cast<HWND>(hwnd_);
    if (owner_) {
        // Bind to the owner: minimize with it, stay above it, and if modal,
        // disable the owner for the lifetime of this window.
        SetWindowLongPtrW(hwnd, GWLP_HWNDPARENT,
                          reinterpret_cast<LONG_PTR>(owner_->native_handle()));
    }
    if (modal_) {
        HWND owner_hwnd = owner_ ? static_cast<HWND>(owner_->native_handle()) : nullptr;
        if (owner_hwnd) EnableWindow(owner_hwnd, FALSE);
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    } else if (topmost_) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
}

Window& Window::set_owner(Window* owner) {
    owner_ = owner;
    if (hwnd_) update_owner_state();
    return *this;
}

Window& Window::set_modal(bool modal) {
    modal_ = modal;
    if (hwnd_) update_owner_state();
    return *this;
}

Window& Window::set_topmost(bool topmost) {
    topmost_ = topmost;
    if (hwnd_) update_owner_state();
    return *this;
}

void Window::destroy() {
    if (!hwnd_) return;
    HWND hwnd = static_cast<HWND>(hwnd_);
    hwnd_ = nullptr;
    hover_ = nullptr;
    capture_ = nullptr;
    focused_ = nullptr;
    kill_all_timers();
    // A modal window re-enables its owner when it goes away.
    if (modal_ && owner_ && owner_->native_handle()) {
        EnableWindow(static_cast<HWND>(owner_->native_handle()), TRUE);
    }
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    backend_->destroy_target();
    DestroyWindow(hwnd);
}

void Window::loop_until_closed() {
    if (!hwnd_) return;
    timeBeginPeriod(1);
    MSG msg{};
    while (hwnd_) {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) break;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        } else {
            pump();
            if (hwnd_) WaitMessage();
        }
    }
    timeEndPeriod(1);
}

void Window::show() {
    if (!hwnd_) return;
    ShowWindow(static_cast<HWND>(hwnd_), SW_SHOW);
    UpdateWindow(static_cast<HWND>(hwnd_));
}

void Window::close() {
    if (!hwnd_ || closing_) return;
    closing_ = true;
    PostMessageW(static_cast<HWND>(hwnd_), WM_CLOSE, 0, 0);
}

Window& Window::set_root(Widget* widget) {
    context_menu_ = nullptr;
    if (root_ && root_ != widget) {
        root_->remove_from_parent();
        // Old root subtree unmounts: clear state pointers into it so the whole tree can die safely
        hover_ = nullptr;
        if (capture_) {
            capture_ = nullptr;
            if (hwnd_) ReleaseCapture();
        }
        focused_ = nullptr;
        drag_source_ = nullptr;
        dragging_ = false;
        press_armed_ = false;
        kill_all_timers();
    }
    root_ = widget;
    if (root_) {
        root_->set_parent(this);
        root_->set_bounds(bounds_);
    }
    invalidate_all();
    return *this;
}

void Window::detach_widget(Widget* widget) {
    if (!widget) return;
    if (hover_ == widget) hover_ = nullptr;
    if (capture_ == widget) {
        capture_ = nullptr;
        if (hwnd_) ReleaseCapture();
    }
    if (focused_ == widget) focused_ = nullptr;
    if (drag_source_ == widget) {
        drag_source_ = nullptr;
        dragging_ = false;
        press_armed_ = false;
    }
    if (context_menu_ == widget) context_menu_ = nullptr;
    stop_timer(widget);
}

Window& Window::set_focus(Widget* widget) {
    if (focused_ == widget) return *this;
    if (focused_) {
        Event lost;
        lost.type = EventType::FocusLost;
        focused_->on_event(lost);
        focused_->invalidate();
    }
    focused_ = widget;
    if (widget) {
        Event got;
        got.type = EventType::FocusGained;
        widget->on_event(got);
        widget->invalidate();
    }
    // The new focus target decides whether composition input is enabled and where
    // the IME UI anchors.
    apply_ime_focus();
    refresh_ime_anchor();
    return *this;
}

namespace {

// Chords the focused text input already "owns": plain alphanumeric/editing keys
// (which Type/Backspace/arrows/etc. consume) plus the Ctrl editing combos. App
// chords (Ctrl+S, Ctrl+Shift+T, Alt+... and similar) are never stolen.
bool is_editing_chord(u32 vk, u8 mods) {
    if (mods == KeyModifier_None) {
        if (vk >= '0' && vk <= '9') return true;
        if (vk >= 'A' && vk <= 'Z') return true;
        switch (vk) {
            case VK_BACK: case VK_DELETE: case VK_LEFT: case VK_RIGHT:
            case VK_UP: case VK_DOWN: case VK_HOME: case VK_END:
            case VK_RETURN: case VK_SPACE: case VK_TAB: case VK_ESCAPE:
            case VK_OEM_1: case VK_OEM_2: case VK_OEM_3: case VK_OEM_4:
            case VK_OEM_5: case VK_OEM_6: case VK_OEM_7: case VK_OEM_PLUS:
            case VK_OEM_MINUS: case VK_OEM_COMMA: case VK_OEM_PERIOD:
                return true;
            default:
                return false;
        }
    }
    if (mods == (KeyModifier_Control | KeyModifier_None)) {
        switch (vk) {
            case 'A': case 'Z': case 'X': case 'C': case 'V': case 'Y':
            case 'K': case 'U':  // delete-to-line-start / delete-line
            case VK_LEFT: case VK_RIGHT: case VK_BACK: case VK_DELETE:
            case VK_HOME: case VK_END:
                return true;  // word nav / word delete / scalar jumps TextBox handles
            default:
                return false;
        }
    }
    return false;
}

}  // namespace

Window& Window::add_accelerator(Accelerator acc) {
    for (auto& a : accelerators_) {
        if (a.vk == acc.vk && a.mods == acc.mods) {
            a.action = std::move(acc.action);
            return *this;
        }
    }
    accelerators_.push_back(std::move(acc));
    return *this;
}

bool Window::remove_accelerator(u32 vk, u8 mods) {
    for (auto it = accelerators_.begin(); it != accelerators_.end(); ++it) {
        if (it->vk == vk && it->mods == mods) {
            accelerators_.erase(it);
            return true;
        }
    }
    return false;
}

bool Window::fire_accelerator(u32 vk, u8 mods) {
    suppress_accelerator_match_ = false;
    for (const auto& a : accelerators_) {
        if (a.vk != vk || a.mods != mods) continue;
        if (!a.action) return true;  // registered but empty: matched, nothing to run
        if (focused_ && focused_->wants_ime() && is_editing_chord(vk, mods)) {
            // The focused text input owns this chord: let it reach the widget as a
            // normal key instead. Still a "match" so we stop processing here.
            suppress_accelerator_match_ = true;
            return true;
        }
        a.action();
        return true;
    }
    return false;
}

void Window::refresh_ime_anchor() {
    if (!hwnd_ || !focused_ || !focused_->wants_ime()) return;
    HIMC himc = ImmGetContext(static_cast<HWND>(hwnd_));
    if (!himc) return;
    const RectF r = focused_->ime_caret_rect();
    if (!r.empty()) {
        COMPOSITIONFORM cf{};
        cf.dwStyle = CFS_POINT;
        cf.ptCurrentPos.x = static_cast<LONG>(r.left);
        cf.ptCurrentPos.y = static_cast<LONG>(r.bottom);
        ImmSetCompositionWindow(himc, &cf);
        CANDIDATEFORM cand{};
        cand.dwIndex = 0;
        cand.dwStyle = CFS_EXCLUDE;
        cand.ptCurrentPos = cf.ptCurrentPos;
        cand.rcArea = RECT{static_cast<LONG>(r.left), static_cast<LONG>(r.top),
                           static_cast<LONG>(r.right), static_cast<LONG>(r.bottom)};
        ImmSetCandidateWindow(himc, &cand);
    }
    ImmReleaseContext(static_cast<HWND>(hwnd_), himc);
}

void Window::apply_ime_focus() {
    if (!hwnd_) return;
    const bool want =
        focused_ && focused_->visible() && focused_->enabled() && focused_->wants_ime();
    if (want && ime_disabled_) {
        ImmAssociateContext(static_cast<HWND>(hwnd_), static_cast<HIMC>(ime_prev_context_));
        ime_prev_context_ = nullptr;
        ime_disabled_ = false;
    } else if (!want && !ime_disabled_) {
        ime_prev_context_ = ImmAssociateContext(static_cast<HWND>(hwnd_), nullptr);
        ime_disabled_ = true;
    }
}

void Window::on_ime_composition(u32 flags) {
    if (!hwnd_ || !focused_) return;
    HIMC himc = ImmGetContext(static_cast<HWND>(hwnd_));
    if (!himc) return;

    const auto read_string = [&](DWORD code) {
        LONG bytes = ImmGetCompositionStringW(himc, code, nullptr, 0);
        if (bytes <= 0) return WString{};
        WString s(static_cast<size_t>(bytes) / sizeof(wchar_t), L'\0');
        ImmGetCompositionStringW(himc, code, s.data(), bytes);
        return s;
    };

    if (flags & GCS_RESULTSTR) {
        ime_text_buffer_ = read_string(GCS_RESULTSTR);
        if (!ime_text_buffer_.empty()) {
            Event e;
            e.type = EventType::ImeCommit;
            e.data.ime.text = ime_text_buffer_.c_str();
            e.data.ime.length = static_cast<u32>(ime_text_buffer_.size());
            focused_->on_event(e);
        }
        refresh_ime_anchor();
    }
    if (flags & GCS_COMPSTR) {
        ime_text_buffer_ = read_string(GCS_COMPSTR);
        LONG cursor = ImmGetCompositionStringW(himc, GCS_CURSORPOS, nullptr, 0);
        if (cursor < 0 || cursor > static_cast<LONG>(ime_text_buffer_.size())) cursor = 0;
        Event e;
        e.type = EventType::ImeCompose;
        e.data.ime.text = ime_text_buffer_.c_str();
        e.data.ime.length = static_cast<u32>(ime_text_buffer_.size());
        e.data.ime.cursor = static_cast<u32>(cursor);
        focused_->on_event(e);
    }
    ImmReleaseContext(static_cast<HWND>(hwnd_), himc);
}

void Window::on_ime_end_composition() {
    // Cancel path (e.g. Esc): clear leftover pre-edit display. After a normal commit
    // this is a harmless no-op.
    if (!focused_) return;
    Event e;
    e.type = EventType::ImeCompose;
    e.data.ime.text = nullptr;
    e.data.ime.length = 0;
    e.data.ime.cursor = 0;
    focused_->on_event(e);
}

void Window::set_debug_overlay(DebugOverlay* overlay) {
    debug_overlay_ = overlay;
}

void Window::toggle_debug_overlay() {
    if (!debug_overlay_) return;
    debug_overlay_->toggle(*this);
}

void Window::set_widget_inspector(WidgetInspector* inspector) {
    widget_inspector_ = inspector;
}

void Window::toggle_widget_inspector() {
    if (!widget_inspector_) return;
    widget_inspector_->toggle(*this);
}

void Window::start_timer(Widget* widget, u32 interval_ms) {
    if (!hwnd_ || !widget) return;
    // One timer per widget: re-starting (e.g. re-arming a one-shot timer) must kill the
    // old one, or each re-arm leaks a resident timer and the growing WM_TIMER flood
    // starves rendering (animations stutter).
    stop_timer(widget);
    const UINT_PTR id = next_timer_id_++;
    timers_[id] = widget;
    SetTimer(static_cast<HWND>(hwnd_), id, interval_ms, nullptr);
}

void Window::stop_timer(Widget* widget) {
    if (!hwnd_ || !widget) return;
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (it->second == widget) {
            KillTimer(static_cast<HWND>(hwnd_), it->first);
            it = timers_.erase(it);
        } else {
            ++it;
        }
    }
}

void Window::kill_all_timers() {
    if (!hwnd_) return;
    for (const auto& [id, _] : timers_) {
        KillTimer(static_cast<HWND>(hwnd_), id);
    }
    timers_.clear();
}

}  // namespace yzk