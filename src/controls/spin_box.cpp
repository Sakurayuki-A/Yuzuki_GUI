#include <yuzuki/controls/spin_box.hpp>
#include <yuzuki/ui/window.hpp>

#include <windows.h>

#include <cstdlib>

namespace yzk {

namespace {

Point to_local(Widget* widget, f32 x, f32 y) {
    const RectF g = widget->global_bounds();
    return Point{x - g.left, y - g.top};
}

}  // namespace

SpinBox::SpinBox() : TextBox(String(), TextBoxConfig{}) {
    set_content_inset(spin_width_);
    sync_text();
}

SpinBox::SpinBox(f64 value, f64 min, f64 max, f64 step)
    : TextBox(String(), TextBoxConfig{}), value_(value), min_(min), max_(max), step_(step) {
    if (min_ > max_) std::swap(min_, max_);
    if (value_ < min_) value_ = min_;
    if (value_ > max_) value_ = max_;
    set_content_inset(spin_width_);
    sync_text();
}

void SpinBox::set_value(f64 value) {
    if (value < min_) value = min_;
    if (value > max_) value = max_;
    if (value_ == value) return;
    value_ = value;
    sync_text();
    on_value_changed(value_);
}

void SpinBox::set_range(f64 min, f64 max) {
    if (min > max) std::swap(min, max);
    min_ = min;
    max_ = max;
    if (value_ < min_ || value_ > max_) set_value(value_);
}

void SpinBox::step_by(i32 dir) {
    const f64 v = parse_value();
    f64 next = v + static_cast<f64>(dir) * step_;
    set_value(next);
}

f64 SpinBox::parse_value() const {
    const String s = text();
    const char* c = s.c_str();
    char* end = nullptr;
    const f64 v = std::strtod(c, &end);
    if (end == c) return value_;
    return v;
}

void SpinBox::sync_text() {
    char buf[64];
    if (decimals_ <= 0) {
        snprintf(buf, sizeof(buf), "%.0f", value_);
    } else {
        snprintf(buf, sizeof(buf), "%.*f", decimals_, value_);
    }
    set_text(buf);
}

Size SpinBox::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{160.0f, 32.0f};
}

void SpinBox::paint_impl(PaintContext& ctx) {
    TextBox::paint_impl(ctx);

    const Theme& theme = ctx.theme();
    const RectF btn = RectF::make(bounds_.right - spin_width_, bounds_.top, spin_width_,
                                  bounds_.height());
    const f32 mid = btn.top + btn.height() * 0.5f;
    const Point c{btn.left + spin_width_ * 0.5f, mid};

    ctx.draw_line(Point{btn.left, btn.top + 4.0f}, Point{btn.left, btn.bottom - 4.0f},
                  theme.border, 1.0f);

    Color up = hover_up_ ? theme.accent : theme.text_secondary;
    Color down = hover_down_ ? theme.accent : theme.text_secondary;
    ctx.draw_line(Point{c.x - 3.5f, mid - 2.0f}, Point{c.x, mid - 6.5f}, up, 1.5f);
    ctx.draw_line(Point{c.x + 3.5f, mid - 2.0f}, Point{c.x, mid - 6.5f}, up, 1.5f);
    ctx.draw_line(Point{c.x - 3.5f, mid + 2.0f}, Point{c.x, mid + 6.5f}, down, 1.5f);
    ctx.draw_line(Point{c.x + 3.5f, mid + 2.0f}, Point{c.x, mid + 6.5f}, down, 1.5f);
}

void SpinBox::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseDown:
            if (e.data.mouse.buttons & MouseButton_Left) {
                const Point p = to_local(this, e.data.mouse.x, e.data.mouse.y);
                if (p.x >= bounds_.width() - spin_width_ &&
                    p.y >= 0.0f && p.y <= bounds_.height()) {
                    if (Window* win = window()) win->set_focus(this);
                    const bool up = p.y < bounds_.height() * 0.5f;
                    step_by(up ? 1 : -1);
                    e.consumed = true;
                    return;
                }
            }
            break;

        case EventType::MouseMove: {
            const Point p = to_local(this, e.data.mouse.x, e.data.mouse.y);
            const bool in_up = p.x >= bounds_.width() - spin_width_ &&
                               p.y >= 0.0f && p.y < bounds_.height() * 0.5f;
            const bool in_down = p.x >= bounds_.width() - spin_width_ &&
                                 p.y >= bounds_.height() * 0.5f && p.y <= bounds_.height();
            if (in_up != hover_up_ || in_down != hover_down_) {
                hover_up_ = in_up;
                hover_down_ = in_down;
                invalidate();
            }
            break;
        }

        case EventType::MouseLeave:
            if (hover_up_ || hover_down_) {
                hover_up_ = false;
                hover_down_ = false;
                invalidate();
            }
            break;

        case EventType::KeyDown:
            if (e.data.key.code == VK_RETURN) {
                set_value(parse_value());
                e.consumed = true;
                return;
            }
            if (e.data.key.code == VK_UP) {
                step_by(1);
                e.consumed = true;
                return;
            }
            if (e.data.key.code == VK_DOWN) {
                step_by(-1);
                e.consumed = true;
                return;
            }
            break;

        default:
            break;
    }
    TextBox::on_event(e);
}

}  // namespace yzk