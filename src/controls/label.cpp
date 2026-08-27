#include <yuzuki/controls/label.hpp>
#include <yuzuki/core/encoding.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

namespace {
f32 estimate_text_width(const String& text, f32 font_size) {
    return static_cast<f32>(utf::to_wide(text).size()) * font_size * 0.55f;
}
}

Label::Label(String text) : text_(std::move(text)) {}

Label& Label::set_text(const String& text) {
    if (text_ == text) return *this;
    text_ = text;
    invalidate();
    return *this;
}

Label& Label::set_text_color(const Color& color) {
    if (text_color_ == color) return *this;
    text_color_ = color;
    invalidate();
    return *this;
}

Label& Label::set_text_role(TextRole role) {
    if (text_role_ == role) return *this;
    text_role_ = role;
    invalidate();
    return *this;
}

Label& Label::set_small(bool small) {
    if (small_ == small) return *this;
    small_ = small;
    invalidate();
    return *this;
}

Label& Label::set_bold(bool bold) {
    if (bold_ == bold) return *this;
    bold_ = bold;
    invalidate();
    return *this;
}

Label& Label::set_align(TextAlignH align_h, TextAlignV align_v) {
    align_h_ = align_h;
    align_v_ = align_v;
    invalidate();
    return *this;
}

Size Label::measure_impl(Size available, const PaintContext* ctx) {
    if (text_.empty()) return Size{2.0f, 4.0f};
    if (ctx) {
        const f32 max_width = available.width > 0.0f ? available.width : 1e7f;
        const Size measured = ctx->measure_text(text_, small_, max_width);
        return Size{measured.width + 2.0f, measured.height};
    }
    const f32 font_size = small_ ? 12.0f : 14.0f;
    return Size{estimate_text_width(text_, font_size) + 2.0f, font_size * 1.4f};
}

void Label::paint_impl(PaintContext& ctx) {
    Color color = text_color_;
    if (color.is_transparent()) {
        const Theme& theme = ctx.theme();
        color = text_role_ == TextRole::Secondary ? theme.text_secondary
              : text_role_ == TextRole::Disabled ? theme.text_disabled
                                                 : theme.text;
    }
    if (small_) {
        ctx.draw_text_small(text_, bounds_, color, align_h_, align_v_);
    } else {
        ctx.draw_text(text_, bounds_, color, align_h_, align_v_);
    }
}

}  // namespace yzk