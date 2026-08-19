#pragma once

#include <yuzuki/ui/widget.hpp>

namespace yzk {

// Box model container: background / radius / border / padding / shadow plus one content child.
// Common appearances (cards, panels, list items) are fully declarative:
//   auto card = new Box;
//   card->set_bg(theme.surface);
//   card->set_radius(12.0f);
//   card->set_border(1.0f, Color{0x0f, 0x0f, 0x0f, 30});
//   card->set_padding(16.0f);
//   card->set_shadow(12.0f, 4.0f);  // blur 12, offset down 4
//   card->append_child(content);
//
// The child fills the content area after padding; without a child the Box keeps its natural size.
// Background/border draw in bounds_, shadow below it (painted_bounds_ auto-expands).
class Box : public Widget {
public:
    Box() = default;
    explicit Box(const Color& bg) : bg_(bg) {}

    void set_bg(const Color& color);
    const Color& bg() const { return bg_; }
    void set_radius(f32 radius);
    f32 radius() const { return radius_; }

    void set_border_width(f32 width);
    f32 border_width() const { return border_width_; }
    void set_border_color(const Color& color);
    const Color& border_color() const { return border_color_; }
    void set_border(f32 width, const Color& color);

    void set_padding(f32 padding);
    f32 padding() const { return padding_; }

    // Shadow: blur radius (DIP), offset_y downward; blur <= 0 disables
    void set_shadow(f32 blur, f32 offset_y = 4.0f);
    void set_shadow_color(const Color& color);
    f32 shadow_blur() const { return shadow_blur_; }
    f32 shadow_offset_y() const { return shadow_offset_y_; }

    // Content area (rect after padding, local coords)
    RectF content_area() const;

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;
    void paint_impl(PaintContext& ctx) override;

private:
    Color bg_{0, 0, 0, 0};
    f32 radius_ = 0.0f;
    f32 border_width_ = 0.0f;
    Color border_color_{0, 0, 0, 0};
    f32 padding_ = 0.0f;
    f32 shadow_blur_ = 0.0f;
    f32 shadow_offset_y_ = 4.0f;
    Color shadow_color_{0, 0, 0, 45};
};

}  // namespace yzk