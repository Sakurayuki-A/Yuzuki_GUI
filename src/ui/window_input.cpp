// Window: input — mouse/wheel/keyboard, event bubbling, hover/cursor, focus navigation.
// Borderless gestures (drag/resize) in window_borderless.cpp; window procedure in window_wndproc.cpp
#include <yuzuki/ui/window.hpp>

#include <yuzuki/controls/tooltip.hpp>
#include <yuzuki/controls/button.hpp>
#include "window_internal.hpp"

#include <algorithm>

namespace yzk {

namespace {

void collect_focusable(Widget* w, std::vector<Widget*>& out) {
    if (!w || !w->visible()) return;
    if (w->focusable()) out.push_back(w);
    Widget* child = w->first_child();
    while (child) {
        collect_focusable(child, out);
        child = child->next_sibling();
    }
}

}  // namespace

Widget* Window::hit_test(f32 x, f32 y) {
    if (!root_) return nullptr;
    return root_->hit_test(x, y);
}

void Window::dispatch(Widget* target, Event& e) {
    e.consumed = false;
    Widget* w = target;
    while (w && !e.consumed) {
        if (w->visible()) w->on_event(e);
        w = w->parent();
    }
}

void Window::update_hover(f32 x, f32 y) {
    Widget* hit = (x < 0.0f || y < 0.0f) ? nullptr : hit_test(x, y);
    if (hit == hover_) return;

    Widget* old = hover_;
    hover_ = hit;

    if (old) {
        Event leave;
        leave.type = EventType::MouseLeave;
        leave.data.mouse = MouseData{last_cursor_pos_x_, last_cursor_pos_y_, MouseButton_None, map_mods()};
        old->on_event(leave);
        // Containers with no self visual need no repaint on hover change: invalidating the
        // container subtree (its painted_bounds_ covers all children) would cost
        // thousands of commands per frame on large UIs.
        if (old->has_self_visual()) old->invalidate();
    }
    if (hit) {
        Event enter;
        enter.type = EventType::MouseEnter;
        enter.data.mouse = MouseData{last_cursor_pos_x_, last_cursor_pos_y_, MouseButton_None, map_mods()};
        hit->on_event(enter);
        if (hit->has_self_visual()) hit->invalidate();
    }
    update_cursor();
}

void Window::update_cursor() {
    if (!hwnd_) return;
    Widget* source = hover_ ? hover_ : (capture_ ? capture_ : nullptr);
    Cursor cursor = source ? source->cursor() : Cursor::Arrow;
    if (cursor == current_cursor_) return;
    current_cursor_ = cursor;
    SetCursor(load_cursor(cursor));
}

void Window::on_mouse_input(u32 message, f32 x_px, f32 y_px, u8 buttons, u8 mods) {
    const f32 scale = backend_ ? backend_->dpi_scale() : 1.0f;
    const f32 x = x_px / scale;
    const f32 y = y_px / scale;

    if (message == WM_MOUSEMOVE) {
        last_cursor_pos_x_ = x;
        last_cursor_pos_y_ = y;

        // Manual borderless drag/resize: the press recorded the window rect and cursor
        // origin; move now applies the delta via SetWindowPos (WM_SIZE updates layout)
        if (resize_zone_ && hwnd_) {
            POINT sp{};
            GetCursorPos(&sp);
            const int dl = sp.x - resize_press_sx_;
            const int dr = sp.y - resize_press_sy_;
            int nl = resize_win_l_, nt = resize_win_t_;
            int nr = resize_win_r_, nb = resize_win_b_;
            switch (resize_zone_) {
                case HTCAPTION:
                    nl += dl; nt += dr; nr += dl; nb += dr;
                    break;
                case HTLEFT: nl += dl; break;
                case HTRIGHT: nr += dl; break;
                case HTTOP: nt += dr; break;
                case HTBOTTOM: nb += dr; break;
                case HTTOPLEFT: nl += dl; nt += dr; break;
                case HTTOPRIGHT: nr += dl; nt += dr; break;
                case HTBOTTOMLEFT: nl += dl; nb += dr; break;
                case HTBOTTOMRIGHT: nr += dl; nb += dr; break;
                default: break;
            }
            const f32 scale = backend_ ? backend_->dpi_scale() : 1.0f;
            const int minw = static_cast<int>(120.0f * scale + 0.5f);
            const int minh = static_cast<int>(60.0f * scale + 0.5f);
            if (nr - nl < minw) {
                if (resize_zone_ == HTLEFT || resize_zone_ == HTTOPLEFT ||
                    resize_zone_ == HTBOTTOMLEFT)
                    nl = nr - minw;
                else
                    nr = nl + minw;
            }
            if (nb - nt < minh) {
                if (resize_zone_ == HTTOP || resize_zone_ == HTTOPLEFT ||
                    resize_zone_ == HTTOPRIGHT)
                    nt = nb - minh;
                else
                    nb = nt + minh;
            }
            SetWindowPos(static_cast<HWND>(hwnd_), nullptr, nl, nt, nr - nl, nb - nt,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            return;
        }

        if (dragging_ && drag_source_) {
            Widget* parent = drag_source_->parent();
            const RectF pg = parent ? parent->global_bounds()
                                    : RectF::make(0.0f, 0.0f,
                                                  static_cast<f32>(client_width_),
                                                  static_cast<f32>(client_height_));
            f32 nx = x - grab_dx_ - pg.left;
            f32 ny = y - grab_dy_ - pg.top;
            nx = nx < 0.0f ? 0.0f : nx;
            ny = ny < 0.0f ? 0.0f : ny;
            if (nx + drag_source_->width() > pg.width()) nx = pg.width() - drag_source_->width();
            if (ny + drag_source_->height() > pg.height()) ny = pg.height() - drag_source_->height();
            const RectF old_gb = drag_source_->global_bounds();
            drag_source_->set_position(nx, ny);
            // set_bounds already invalidated old/new bodies + old visual extent (incl. shadow);
            // also invalidate the new position's shadow spill, derived from painted_bounds_
            const RectF gb = drag_source_->global_bounds();
            const RectF pb = drag_source_->painted_bounds();
            RectF inv = gb;
            if (!pb.empty()) {
                const f32 pad_x =
                    std::max(pb.width() - old_gb.width(), 0.0f) * 0.5f;
                const f32 pad_y =
                    std::max(pb.height() - old_gb.height(), 0.0f) * 0.5f;
                inv = inv.inflated(pad_x, pad_y);
            }
            invalidate_area(inv.inflated(24.0f, 24.0f));
            Event drag;
            drag.type = EventType::DragMove;
            drag.data.mouse = MouseData{x, y, buttons, mods};
            dispatch(drag_source_, drag);
            return;
        }

        if (!dragging_ && press_armed_ && capture_ && capture_->draggable() &&
            capture_->visible()) {
            const f32 threshold = 5.0f;
            if (std::abs(x - press_x_) > threshold || std::abs(y - press_y_) > threshold) {
                dragging_ = true;
                drag_source_ = capture_;
                const RectF gb = capture_->global_bounds();
                grab_dx_ = press_x_ - gb.left;
                grab_dy_ = press_y_ - gb.top;
                Event start;
                start.type = EventType::DragStart;
                start.data.mouse = MouseData{x, y, buttons, mods};
                dispatch(capture_, start);
            }
        }

        update_hover(x, y);
        Widget* target = capture_ ? capture_ : hover_;
        if (target) {
            Event move;
            move.type = EventType::MouseMove;
            move.data.mouse = MouseData{x, y, buttons, mods};
            dispatch(target, move);
        }
        TooltipManager::instance().on_hover_changed(*this, hover_, x, y);
        TooltipManager::instance().on_mouse_move(*this, x, y);
        return;
    }

    if (message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK ||
        message == WM_RBUTTONDOWN || message == WM_RBUTTONDBLCLK ||
        message == WM_MBUTTONDOWN || message == WM_MBUTTONDBLCLK) {
        // Borderless: pressing an edge zone or blank caption starts a manual drag/resize.
        // The system hit cache reports HTCLIENT for client areas (no WM_NCLBUTTONDOWN
        // arrives), so it is driven ourselves (SetCapture + SetWindowPos on move).
        if ((message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK) && borderless_ &&
            !maximized() && !resize_zone_) {
            const int zone = hit_zone(x, y);
            if (zone != HTCLIENT || caption_hit(x, y)) {
                start_gesture(zone != HTCLIENT ? zone : HTCAPTION);
                return;
            }
        }
        if (context_menu_ && context_menu_->is_open()) {
            if (hit_test(x, y) == context_menu_) {
                Event down;
                down.type = EventType::MouseDown;
                down.data.mouse = MouseData{x, y, buttons, mods};
                dispatch(context_menu_, down);
                return;
            }
            context_menu_->close();
            return;
        }
        Widget* target = capture_ ? capture_ : hit_test(x, y);
        if (!capture_) {
            capture_ = target;
            SetCapture(static_cast<HWND>(hwnd_));
        }
        press_x_ = x;
        press_y_ = y;
        press_armed_ = true;
        if (target && target->focusable()) {
            set_focus(target);
            focus_visible_ = false;  // mouse clicks don't show the focus ring
        }
        Event down;
        down.type = EventType::MouseDown;
        down.data.mouse = MouseData{x, y, buttons, mods};
        dispatch(target, down);
        if (context_menu_ && context_menu_->is_open() && capture_ == target) {
            capture_ = nullptr;
            ReleaseCapture();
        }
        update_cursor();
        return;
    }

    if (message == WM_LBUTTONUP || message == WM_RBUTTONUP || message == WM_MBUTTONUP) {
        if (resize_zone_) {
            resize_zone_ = 0;
            ReleaseCapture();
            return;
        }
        Widget* target = capture_;
        if (!target) return;
        if (dragging_ && drag_source_) {
            Event end;
            end.type = EventType::DragEnd;
            end.data.mouse = MouseData{x, y, buttons, mods};
            dispatch(drag_source_, end);
        }
        dragging_ = false;
        drag_source_ = nullptr;
        press_armed_ = false;
        Event up;
        up.type = EventType::MouseUp;
        up.data.mouse = MouseData{x, y, buttons, mods};
        dispatch(target, up);

        if (hit_test(x, y) == target) {
            Event click;
            click.type = EventType::Click;
            click.data.mouse = MouseData{x, y, buttons, mods};
            dispatch(target, click);
        }
        capture_ = nullptr;
        ReleaseCapture();
        update_hover(x, y);
        update_cursor();
        return;
    }
}

void Window::on_wheel(f32 x_dip, f32 y_dip, i16 delta, u8 mods) {
    Widget* target = hit_test(x_dip, y_dip);
    if (!target) return;
    Event wheel;
    wheel.type = EventType::Wheel;
    wheel.data.mouse = MouseData{x_dip, y_dip, MouseButton_None, mods, delta};
    dispatch(target, wheel);
}

void Window::focus_next(bool reverse) {
    focus_visible_ = true;  // keyboard navigation shows the focus ring
    std::vector<Widget*> order;
    collect_focusable(root_, order);
    if (order.empty()) return;

    const auto it = std::find(order.begin(), order.end(), focused_);
    if (it == order.end()) {
        set_focus(order.front());
        return;
    }
    const size_t idx = static_cast<size_t>(it - order.begin());
    const size_t n = order.size();
    set_focus(order[(idx + (reverse ? n - 1 : 1)) % n]);
}

void Window::activate_focused() {
    if (!focused_) return;
    focus_visible_ = true;  // Enter activation is a keyboard path too
    Event key;
    key.type = EventType::KeyDown;
    key.data.key = KeyData{VK_RETURN, 0, 0, false};
    dispatch(focused_, key);
    if (key.consumed) return;
    if (auto* button = dynamic_cast<Button*>(focused_)) button->activate();
}

void Window::on_key(u32 message, u32 vk, u16 chr, u8 mods, bool repeat) {
    if (message == WM_KEYDOWN && vk == VK_TAB) {
        focus_next((GetKeyState(VK_SHIFT) & 0x8000) != 0);
        return;
    }
    if (message == WM_KEYDOWN && vk == VK_RETURN) {
        activate_focused();
        return;
    }
    if (message == WM_KEYDOWN && vk == VK_ESCAPE && context_menu_) {
        context_menu_->close();
        return;
    }
    Widget* target = focused_ ? focused_ : hover_;
    if (!target) return;
    Event key;
    if (message == WM_CHAR) {
        key.type = EventType::Character;
        key.data.key = KeyData{0, chr, mods, repeat};
    } else {
        key.type = message == WM_KEYDOWN ? EventType::KeyDown : EventType::KeyUp;
        key.data.key = KeyData{vk, chr, mods, repeat};
    }
    dispatch(target, key);
}

}  // namespace yzk