// Window: lifecycle / widget-tree mounting / focus state / timers.
// Rendering, input, borderless and the window procedure live in window_frame.cpp /
// window_input.cpp / window_borderless.cpp / window_wndproc.cpp
#include <yuzuki/ui/window.hpp>

#include <yuzuki/ui/application.hpp>
#include <yuzuki/core/encoding.hpp>
#include "ui/window_internal.hpp"
#include "render/d2d/d2d_backend.hpp"

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

void Window::set_title(const String& title) {
    title_ = title;
    if (hwnd_) SetWindowTextW(static_cast<HWND>(hwnd_), utf::to_wide(title_).c_str());
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