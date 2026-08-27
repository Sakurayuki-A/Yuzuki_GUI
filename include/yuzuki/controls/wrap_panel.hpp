#pragma once
#include <yuzuki/controls/layout.hpp>

#include <vector>

namespace yzk {

class WrapPanel : public Layout {
public:
    WrapPanel() = default;

    f32 spacing() const { return spacing_; }
    WrapPanel& set_spacing(f32 spacing);
    WrapPanel& spacing(f32 spacing) { return set_spacing(spacing); }

    f32 line_spacing() const { return line_spacing_; }
    WrapPanel& set_line_spacing(f32 line_spacing);

    Size measure_content(Size available, const PaintContext* ctx) override;
    void arrange_content(const RectF& area, const PaintContext* ctx) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;

private:
    std::vector<f32> child_x_;
    std::vector<f32> child_y_;
    std::vector<Size> child_size_;
    f32 spacing_ = 8.0f;
    f32 line_spacing_ = 8.0f;
};

}  // namespace yzk