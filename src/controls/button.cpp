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

Size Button::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    if (icon_ != IconId::None && text_.empty()) {
        const f32 side = icon_size_ + (padding_ * 2.0f);
        return Size{side, side};
    }
    f32 w = min_width_;
    if (icon_ != IconId::None && ctx) {
        w += icon_size_ + 6.0f;  // offset_t icon + gap + original min width
    }
    return Size{w, 32.0f};
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

    if (icon_ != IconId::None) {
        const f32 icon_w = text_.empty() ? icon_size_ : icon_size_ + 6.0f;
        const f32 icon_left = text_.empty() ? bounds_.left + (bounds_.width() - icon_size_) / 2.0f
                                            : bounds_.left + padding_;
        ctx.draw_icon(icon_, RectF::make(icon_left, bounds_.top, icon_size_, bounds_.height()),
                      text_color, icon_size_);
        if (text_.empty()) return;
        ctx.draw_text(text_, RectF::make(bounds_.left + padding_ + icon_w, bounds_.top,
                                         bounds_.width() - (padding_ + icon_w) * 2.0f,
                                         bounds_.height()),
                      text_color);
    } else {
        ctx.draw_text(text_, bounds_, text_color);
    }
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