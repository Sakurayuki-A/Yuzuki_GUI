#include <yuzuki/controls/button.hpp>

namespace yzk {

namespace {
Point to_local(Widget* widget, f32 x, f32 y) {
    const RectF g = widget->global_bounds();
    return Point{x - g.left, y - g.top};
}
}

Button::Button(String text) : text_(std::move(text)) {
    set_cursor(Cursor::Hand);
    set_focusable(true);
}

void Button::set_text(const String& text) {
    if (text_ == text) return;
    text_ = text;
    invalidate();
}

Size Button::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{min_width_, 32.0f};
}

void Button::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const f32 radius = theme.corner_radius;
    const bool enabled = this->enabled();

    Color fill = accent_ ? theme.accent : theme.surface;
    Color border = accent_ ? theme.accent : theme.border;
    Color text_color = accent_ ? theme.accent_text : theme.text;

    if (!enabled) {
        fill = accent_ ? theme.accent_disabled : theme.surface;
        border = theme.border;
        text_color = theme.text_disabled;
    } else if (has_flag(Flag_Pressed)) {
        fill = accent_ ? theme.accent_pressed : theme.accent_hover;
        border = accent_ ? theme.accent_pressed : theme.border_hover;
    } else if (has_flag(Flag_Hovered)) {
        fill = accent_ ? theme.accent_hover : theme.surface;
        border = theme.border_hover;
        if (!accent_) text_color = theme.accent;
    }

    ctx.fill_rounded(bounds_, fill, radius);
    ctx.draw_border(bounds_, border, 1.0f, radius);
    ctx.draw_text(text_, bounds_, text_color);
}

void Button::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseEnter:
            add_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseLeave:
            remove_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseDown:
            if (enabled() && e.data.mouse.buttons & MouseButton_Left) {
                const Point p = to_local(this, e.data.mouse.x, e.data.mouse.y);
                if (p.x >= 0.0f && p.y >= 0.0f && p.x <= bounds_.width() && p.y <= bounds_.height()) {
                    add_flag(Flag_Pressed);
                    invalidate();
                    e.consumed = true;
                }
            }
            break;

        case EventType::MouseUp:
            if (has_flag(Flag_Pressed)) {
                const Point p = to_local(this, e.data.mouse.x, e.data.mouse.y);
                const bool inside = p.x >= 0.0f && p.y >= 0.0f && p.x <= bounds_.width() && p.y <= bounds_.height();
                remove_flag(Flag_Pressed);
                invalidate();
                if (inside) on_click();
                e.consumed = true;
            }
            break;

        case EventType::Click:
            e.consumed = true;
            break;

        default:
            break;
    }
}

}  // namespace yzk