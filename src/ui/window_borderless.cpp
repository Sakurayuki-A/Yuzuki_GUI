// Window: borderless — style switching / edge resize zones / caption drag / manual gestures / maximize.
#include <yuzuki/ui/window.hpp>

namespace yzk {

void Window::set_borderless(bool borderless) {
    if (borderless_ == borderless) return;
    borderless_ = borderless;
    if (!hwnd_) return;
    HWND hwnd = static_cast<HWND>(hwnd_);
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    style &= ~(WS_OVERLAPPEDWINDOW | WS_POPUP | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU);
    style |= borderless ? (WS_POPUP | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU)
                        : WS_OVERLAPPEDWINDOW;
    SetWindowLongW(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    invalidate_all();
}

void Window::set_caption(Widget* widget) {
    caption_ = widget;
}

void Window::minimize() {
    if (hwnd_) ShowWindow(static_cast<HWND>(hwnd_), SW_MINIMIZE);
}

void Window::maximize_toggle() {
    if (!hwnd_) return;
    HWND hwnd = static_cast<HWND>(hwnd_);
    if (IsZoomed(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    } else {
        ShowWindow(hwnd, SW_MAXIMIZE);
    }
}

bool Window::maximized() const {
    return hwnd_ && IsZoomed(static_cast<HWND>(hwnd_));
}

bool Window::caption_hit(f32 x, f32 y) const {
    if (!caption_ || !caption_->visible()) return false;
    const RectF gb = caption_->global_bounds();
    if (!gb.contains(x, y)) return false;
    // Deepest hit that is not focusable (labels/panels) counts as blank caption →
    // draggable; focusable controls (buttons/text boxes) keep receiving mouse events.
    Widget* hit = caption_->hit_test(x - gb.left, y - gb.top);
    return hit == nullptr || !hit->focusable();
}

void Window::start_gesture(int zone) {
    if (!hwnd_) return;
    RECT wr{};
    GetWindowRect(static_cast<HWND>(hwnd_), &wr);
    resize_win_l_ = wr.left;
    resize_win_t_ = wr.top;
    resize_win_r_ = wr.right;
    resize_win_b_ = wr.bottom;
    POINT sp{};
    GetCursorPos(&sp);
    resize_press_sx_ = sp.x;
    resize_press_sy_ = sp.y;
    resize_zone_ = zone;
    SetCapture(static_cast<HWND>(hwnd_));
}

int Window::hit_zone(f32 x, f32 y) const {
    if (!borderless_ || maximized()) return HTCLIENT;
    const f32 w = bounds_.width();
    const f32 h = bounds_.height();
    constexpr f32 kResizeMargin = 8.0f;
    const bool l = x <= kResizeMargin;
    const bool r = x >= w - kResizeMargin;
    const bool t = y <= kResizeMargin;
    const bool b = y >= h - kResizeMargin;
    if (l && t) return HTTOPLEFT;
    if (r && t) return HTTOPRIGHT;
    if (l && b) return HTBOTTOMLEFT;
    if (r && b) return HTBOTTOMRIGHT;
    if (l) return HTLEFT;
    if (r) return HTRIGHT;
    if (t) return HTTOP;
    if (b) return HTBOTTOM;
    return HTCLIENT;
}

}  // namespace yzk