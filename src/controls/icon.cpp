#include <yuzuki/controls/icon.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

Icon::Icon(IconId id, f32 size) : id_(id), size_(size) {}

void Icon::set_icon(IconId id) {
    if (id_ == id) return;
    id_ = id;
    invalidate();
}

void Icon::set_icon_size(f32 size) {
    if (size_ == size) return;
    size_ = size;
    invalidate();
}

void Icon::set_color(const Color& color) {
    color_ = color;
    invalidate();
}

Size Icon::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{size_, size_};
}

void Icon::paint_impl(PaintContext& ctx) {
    if (id_ == IconId::None) return;
    Color color = color_;
    if (color.is_transparent()) color = ctx.theme().text;
    const FontId font = ctx.font(icon_family, size_);
    ctx.draw_text(font, icon_glyph(id_), bounds_, color);
}

}  // namespace yzk