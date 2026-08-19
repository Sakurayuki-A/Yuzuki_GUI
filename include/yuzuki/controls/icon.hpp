#pragma once
#include <yuzuki/ui/icon.hpp>
#include <yuzuki/ui/widget.hpp>

namespace yzk {

// Icon widget: draws a glyph from the icon font, vector-scaled, color/size configurable
class Icon : public Widget {
public:
    Icon(IconId id, f32 size = 16.0f);

    void set_icon(IconId id);
    IconId icon() const { return id_; }

    void set_icon_size(f32 size);
    f32 icon_size() const { return size_; }

    // Transparent (default) = follow the theme text color
    void set_color(const Color& color);
    const Color& color() const { return color_; }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;

private:
    IconId id_;
    f32 size_ = 16.0f;
    Color color_{0, 0, 0, 0};
};

}  // namespace yzk