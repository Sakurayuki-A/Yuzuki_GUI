// Window: lifecycle / widget-tree mounting / focus state / timers.
// Rendering, input, borderless and the window procedure live in window_frame.cpp /
// window_input.cpp / window_borderless.cpp / window_wndproc.cpp
#include <yuzuki/ui/window.hpp>

#include <yuzuki/ui/application.hpp>
#include <yuzuki/core/encoding.hpp>
#include "ui/window_internal.hpp"
#include "render/d2d/d2d_backend.hpp"

#include <imm.h>

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
    HWND hwnd = CreateWindowExW(
        0, kWindowClass, utf::to_wide(title_).c_str(), style,
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, nullptr, nullptr, hinst, nullptr);
    if (!hwnd) return false;

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    hwnd_ = hwnd;

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
    invalidate_all();
    return true;
}

void Window::destroy() {
    if (!hwnd_) return;
    HWND hwnd = static_cast<HWND>(hwnd_);
    hwnd_ = nullptr;
    hover_ = nullptr;
    capture_ = nullptr;
    focused_ = nullptr;
    kill_all_timers();
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    backend_->destroy_target();
    DestroyWindow(hwnd);
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

void Window::set_root(Widget* widget) {
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

void Window::set_focus(Widget* widget) {
    if (focused_ == widget) return;
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