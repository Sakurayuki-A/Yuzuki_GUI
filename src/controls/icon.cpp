#include <yuzuki/controls/icon.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

Icon::Icon(IconId id, f32 size) : id_(id), size_(size) {}

Icon& Icon::set_icon(IconId id) {
    if (id_ == id) return *this;
    id_ = id;
    invalidate();
    return *this;
}

Icon& Icon::set_icon_size(f32 size) {
    if (size_ == size) return *this;
    size_ = size;
    invalidate();
    return *this;
}

Icon& Icon::set_color(const Color& color) {
    color_ = color;
    invalidate();
    return *this;
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
    ctx.draw_icon(id_, bounds_, color, size_);
}

}  // namespace yzk