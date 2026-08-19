#pragma once
#include <yuzuki/controls/layout.hpp>

namespace yzk {

enum class Orientation : u8 { Horizontal, Vertical };

class StackPanel : public Layout {
public:
    explicit StackPanel(Orientation orientation = Orientation::Vertical);

    Orientation orientation() const { return orientation_; }
    void set_orientation(Orientation orientation);

    f32 spacing() const { return spacing_; }
    void set_spacing(f32 spacing);

    bool stretch_children() const { return stretch_; }
    void set_stretch_children(bool stretch);

    Size measure_content(Size available, const PaintContext* ctx) override;
    void arrange_content(const RectF& area, const PaintContext* ctx) override;

private:
    Orientation orientation_ = Orientation::Vertical;
    f32 spacing_ = 8.0f;
    bool stretch_ = true;
};

}  // namespace yzk