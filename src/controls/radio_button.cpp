#include <yuzuki/controls/radio_button.hpp>

namespace yzk {

namespace {
constexpr f32 kDotSize = 18.0f;
constexpr f32 kHeight = 28.0f;
}

RadioButton::RadioButton(String text) : text_(std::move(text)) {
    set_focusable(true);
    set_cursor(Cursor::Hand);
}

void RadioButton::set_text(const String& text) {
    if (text_ == text) return;
    text_ = text;
    invalidate();
}

void RadioButton::set_checked(bool checked) {
    if (checked_ == checked) return;
    if (checked) check_siblings();
    checked_ = checked;
    invalidate();
}

Size RadioButton::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    f32 text_w = 0.0f;
    if (ctx && !text_.empty()) text_w = ctx->measure_text(text_).width;
    return Size{kDotSize + 8.0f + text_w, kHeight};
}

void RadioButton::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const f32 y = bounds_.top + (bounds_.height() - kDotSize) / 2.0f;
    const RectF dot = RectF::make(bounds_.left, y, kDotSize, kDotSize);
    const f32 radius = kDotSize / 2.0f;

    ctx.fill_rounded(dot, checked_ ? theme.accent : theme.surface, radius);
    ctx.draw_border(dot, checked_ ? theme.accent : theme.border, 1.0f, radius);

    if (checked_) {
        const f32 inner = 6.0f;
        const f32 ix = dot.left + (kDotSize - inner) / 2.0f;
        const f32 iy = dot.top + (kDotSize - inner) / 2.0f;
        ctx.fill_rounded(RectF::make(ix, iy, inner, inner), theme.accent_text, inner / 2.0f);
    }

    if (!text_.empty()) {
        const RectF tr = RectF::make(dot.right + 8.0f, bounds_.top, bounds_.width() - kDotSize - 8.0f, bounds_.height());
        ctx.draw_text(text_, tr, enabled() ? theme.text : theme.text_disabled, TextAlignH::Left, TextAlignV::Center);
    }
}

void RadioButton::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseDown:
            if (enabled() && (e.data.mouse.buttons & MouseButton_Left)) {
                set_checked(true);
                on_toggled(true);
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

void RadioButton::check_siblings() {
    Widget* parent = this->parent();
    if (!parent) return;
    for (Widget* s = parent->first_child(); s; s = s->next_sibling()) {
        if (s == this) continue;
        auto* rb = dynamic_cast<RadioButton*>(s);
        if (rb) rb->set_checked(false);
    }
}

}  // namespace yzk
