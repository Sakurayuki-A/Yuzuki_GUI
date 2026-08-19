#include <yuzuki/controls/toggle_switch.hpp>

namespace yzk {

namespace {
constexpr f32 kTrackWidth = 40.0f;
constexpr f32 kTrackHeight = 20.0f;
constexpr f32 kThumbSize = 16.0f;
constexpr f32 kHeight = 24.0f;
}

ToggleSwitch::ToggleSwitch() {
    set_focusable(true);
}

void ToggleSwitch::set_checked(bool checked) {
    if (checked_ == checked) return;
    checked_ = checked;
    invalidate();
}

Size ToggleSwitch::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{kTrackWidth, kHeight};
}

void ToggleSwitch::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const f32 y = bounds_.top + (bounds_.height() - kTrackHeight) / 2.0f;
    const RectF track = RectF::make(bounds_.left, y, kTrackWidth, kTrackHeight);

    const Color track_color = checked_ ? theme.accent : theme.track;
    ctx.fill_rounded(track, track_color, kTrackHeight / 2.0f);
    if (!checked_) ctx.draw_border(track, theme.border, 1.0f, kTrackHeight / 2.0f);

    const f32 thumb_x = checked_ ? track.right - kThumbSize - 2.0f : track.left + 2.0f;
    const f32 thumb_y = track.top + (kTrackHeight - kThumbSize) / 2.0f;
    ctx.fill_rounded(RectF::make(thumb_x, thumb_y, kThumbSize, kThumbSize), theme.accent_text,
                     kThumbSize / 2.0f);
}

void ToggleSwitch::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseDown:
            if (enabled() && (e.data.mouse.buttons & MouseButton_Left)) {
                set_checked(!checked_);
                on_toggled(checked_);
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

}  // namespace yzk
