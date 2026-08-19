#include <yuzuki/controls/slider.hpp>

namespace yzk {

namespace {
constexpr f32 kHeight = 24.0f;
constexpr f32 kTrackHeight = 4.0f;
constexpr f32 kThumbSize = 14.0f;
constexpr f32 kMinWidth = 120.0f;
}

Slider::Slider() {
    set_focusable(true);
}

void Slider::set_range(f32 min, f32 max) {
    if (max < min) max = min;
    min_ = min;
    max_ = max;
    if (value_ < min_) value_ = min_;
    if (value_ > max_) value_ = max_;
    invalidate();
}

void Slider::set_value(f32 value) {
    if (value < min_) value = min_;
    if (value > max_) value = max_;
    if (value_ == value) return;
    value_ = value;
    invalidate();
}

Size Slider::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{kMinWidth, kHeight};
}

void Slider::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const f32 track_w = bounds_.width() - kThumbSize;
    const f32 track_y = bounds_.top + (bounds_.height() - kTrackHeight) / 2.0f;
    const f32 track_x = bounds_.left + kThumbSize / 2.0f;

    const f32 frac = (max_ > min_) ? (value_ - min_) / (max_ - min_) : 0.0f;
    const f32 fill_w = track_w * frac;

    ctx.fill_rounded(RectF::make(track_x, track_y, track_w, kTrackHeight), theme.track, 2.0f);
    if (fill_w > 0.0f) {
        ctx.fill_rounded(RectF::make(track_x, track_y, fill_w, kTrackHeight), theme.accent, 2.0f);
    }

    const f32 cx = track_x + fill_w;
    const f32 cy = track_y + kTrackHeight / 2.0f;
    const Color thumb_color = dragging_ ? theme.accent_pressed : (has_flag(Flag_Hovered) ? theme.accent_hover : theme.accent);
    ctx.fill_rounded(RectF::make(cx - 3.0f, cy - 9.0f, 6.0f, 18.0f), thumb_color, 3.0f);
}

void Slider::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseDown:
            if (enabled() && (e.data.mouse.buttons & MouseButton_Left)) {
                dragging_ = true;
                const f32 v = value_at_x(e.data.mouse.x);
                if (v != value_) {
                    value_ = v;
                    on_changed(value_);
                }
                invalidate();
                e.consumed = true;
            }
            break;

        case EventType::MouseMove:
            if (dragging_) {
                const f32 v = value_at_x(e.data.mouse.x);
                if (v != value_) {
                    value_ = v;
                    on_changed(value_);
                }
                invalidate();
                e.consumed = true;
            }
            break;

        case EventType::MouseUp:
            if (dragging_) {
                dragging_ = false;
                invalidate();
                e.consumed = true;
            }
            break;

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

        default:
            break;
    }
}

f32 Slider::value_at_x(f32 x) const {
    const f32 track_x = bounds_.left + kThumbSize / 2.0f;
    const f32 track_w = bounds_.width() - kThumbSize;
    const f32 frac = track_w > 0.0f ? (x - track_x) / track_w : 0.0f;
    f32 clamped = frac < 0.0f ? 0.0f : (frac > 1.0f ? 1.0f : frac);
    return min_ + clamped * (max_ - min_);
}

}  // namespace yzk
