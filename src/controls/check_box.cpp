#include <yuzuki/controls/check_box.hpp>

namespace yzk {

namespace {
constexpr f32 kBoxSize = 18.0f;
constexpr f32 kHeight = 28.0f;
}

CheckBox::CheckBox(String text) : text_(std::move(text)) {
    set_focusable(true);
    set_cursor(Cursor::Hand);
}

void CheckBox::set_text(const String& text) {
    if (text_ == text) return;
    text_ = text;
    invalidate();
}

void CheckBox::set_checked(bool checked) {
    if (checked_ == checked) return;
    checked_ = checked;
    invalidate();
}

Size CheckBox::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    f32 text_w = 0.0f;
    if (ctx && !text_.empty()) text_w = ctx->measure_text(text_).width;
    return Size{kBoxSize + 8.0f + text_w, kHeight};
}

void CheckBox::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const f32 y = bounds_.top + (bounds_.height() - kBoxSize) / 2.0f;
    const RectF box = RectF::make(bounds_.left, y, kBoxSize, kBoxSize);

    const Color fill = checked_ ? theme.accent : theme.surface;
    const Color border = checked_ ? theme.accent : (has_flag(Flag_Hovered) ? theme.border_hover : theme.border);

    ctx.fill_rounded(box, fill, 4.0f);
    ctx.draw_border(box, border, 1.0f, 4.0f);

    if (checked_) {
        const f32 cx = box.left + 4.0f;
        const f32 cy = box.top + 4.0f;
        ctx.draw_line(Point{cx, cy + 4.0f}, Point{cx + 4.0f, cy + 8.0f}, theme.accent_text, 2.0f);
        ctx.draw_line(Point{cx + 4.0f, cy + 8.0f}, Point{cx + 10.0f, cy}, theme.accent_text, 2.0f);
    }

    if (!text_.empty()) {
        const RectF tr = RectF::make(box.right + 8.0f, bounds_.top, bounds_.width() - kBoxSize - 8.0f, bounds_.height());
        ctx.draw_text(text_, tr, enabled() ? theme.text : theme.text_disabled, TextAlignH::Left, TextAlignV::Center);
    }
}

void CheckBox::on_event(Event& e) {
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
