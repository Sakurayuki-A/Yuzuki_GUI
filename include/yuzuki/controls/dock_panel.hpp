#pragma once
#include <yuzuki/controls/layout.hpp>

#include <vector>

namespace yzk {

enum class Dock : u8 { Left, Top, Right, Bottom, Fill };

class DockPanel : public Layout {
public:
    DockPanel() = default;

    void dock(Widget* child, Dock dock);

    f32 gap() const { return gap_; }
    void set_gap(f32 gap);

    Size measure_content(Size available, const PaintContext* ctx) override;
    void arrange_content(const RectF& area, const PaintContext* ctx) override;

private:
    std::vector<Dock> docks_;
    f32 gap_ = 0.0f;
};

}  // namespace yzk