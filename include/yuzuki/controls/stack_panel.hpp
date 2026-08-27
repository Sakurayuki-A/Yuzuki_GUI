#pragma once
#include <yuzuki/controls/layout.hpp>

#include <vector>

namespace yzk {

enum class Orientation : u8 { Horizontal, Vertical };

class StackPanel : public Layout {
public:
    explicit StackPanel(Orientation orientation = Orientation::Vertical);

    Orientation orientation() const { return orientation_; }
    StackPanel& set_orientation(Orientation orientation);

    f32 spacing() const { return spacing_; }
    StackPanel& set_spacing(f32 spacing);
    StackPanel& spacing(f32 spacing) { return set_spacing(spacing); }

    bool stretch_children() const { return stretch_; }
    StackPanel& set_stretch_children(bool stretch);

    // Main-axis fill: when the panel has free space along the main axis, children
    // with flex_grow() > 0 split it (flex-grow semantics, like FlexBox).
    StackPanel& set_fill(bool fill) {
        if (fill_ == fill) return *this;
        fill_ = fill;
        invalidate();
        return *this;
    }
    bool fill() const { return fill_; }

    Size measure_content(Size available, const PaintContext* ctx) override;
    void arrange_content(const RectF& area, const PaintContext* ctx) override;

private:
    Orientation orientation_ = Orientation::Vertical;
    f32 spacing_ = 8.0f;
    bool stretch_ = true;
    bool fill_ = false;
};

}  // namespace yzk