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
    set_min_width(80.0f);
}

Size Button::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    if (icon_ != IconId::None && text_.empty()) {
        const f32 side = icon_size_ + (padding_ * 2.0f);
        return Size{side, side};
    }
    const f32 height = ctx ? ctx->theme().control_height : 32.0f;
    // Icon width is added on top of min_width, so callers keep the text width
    // semantic: a "Send" icon button of min_width W renders W + icon + gap wide.
    f32 w = min_size().width;
    if (icon_ != IconId::None && ctx) {
        w += icon_size_ + 6.0f;
    }
    return Size{w, height};
}

void Button::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const f32 radius = theme.control_radius;
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
    ctx.draw_border(bounds_, border, theme.border_width, radius);

    if (icon_ != IconId::None) {
        const f32 icon_w = text_.empty() ? icon_size_ : icon_size_ + 6.0f;
        if (text_.empty()) {
            ctx.draw_icon(icon_, RectF::make(bounds_.left + (bounds_.width() - icon_size_) * 0.5f,
                                             bounds_.top, icon_size_, bounds_.height()),
                          text_color, icon_size_);
            return;
        }
        // Icon + text compose as one group, centered as a whole in the button.
        const f32 text_w = ctx.measure_text(text_).width;
        const f32 group_w = icon_w + text_w;
        const f32 group_left = bounds_.left + (bounds_.width() - group_w) * 0.5f;
        // Icon keeps its own square box centered vertically (the row height may be
        // narrower than the default control height, e.g. inside a compact tool bar).
        const f32 icon_top = bounds_.top + (bounds_.height() - icon_size_) * 0.5f;
        ctx.draw_icon(icon_, RectF::make(group_left, icon_top, icon_size_, icon_size_),
                      text_color, icon_size_);
        ctx.draw_text(text_, RectF::make(group_left + icon_w, bounds_.top,
                                         text_w + 4.0f, bounds_.height()),
                      text_color, TextAlignH::Left, TextAlignV::Center);
    } else {
        ctx.draw_text(text_, bounds_, text_color);
    }
}

void Button::on_event(Event& e) {
    switch (e.type) {
        // Flag_Hovered is maintained centrally by Window::update_hover, but the
        // control also handles enter/leave itself so synthetic/host-driven events
        // (unit tests, direct on_event calls) keep hovered() working.
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