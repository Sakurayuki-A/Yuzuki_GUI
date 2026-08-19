#pragma once
#include <yuzuki/controls/layout.hpp>
#include <yuzuki/controls/stack_panel.hpp>  // Orientation

namespace yzk {

// Main-axis alignment of children (SpaceBetween/SpaceAround degrade to Start when no free space)
enum class FlexAlign : u8 { Start, Center, End, SpaceBetween, SpaceAround };

// Cross-axis alignment of children
enum class FlexCrossAlign : u8 { Start, Center, End, Stretch };

// Flex layout container; children stretch via set_flex_grow / set_flex_shrink:
//   grow   distributes extra space proportionally (0 = fixed)
//   shrink shrinks proportionally when space is tight (0 = fixed; default 1)
// Default alignment: main Start + cross Start (origin at top-left), no stretching.
// Usage:
//   auto row = new FlexBox;                       // horizontal (CSS row semantics)
//   row->set_align_main(FlexAlign::SpaceBetween); // justify between
//   row->set_align_cross(FlexCrossAlign::Center); // cross-axis center
//   title->set_flex_grow(1);                      // absorbs all extra width
class FlexBox : public Layout {
public:
    explicit FlexBox(Orientation direction = Orientation::Horizontal);

    Orientation direction() const { return direction_; }
    void set_direction(Orientation direction);

    f32 spacing() const { return spacing_; }
    void set_spacing(f32 spacing);

    FlexAlign main_align() const { return main_align_; }
    void set_align_main(FlexAlign align);

    FlexCrossAlign cross_align() const { return cross_align_; }
    void set_align_cross(FlexCrossAlign align);

    Size measure_content(Size available, const PaintContext* ctx) override;
    void arrange_content(const RectF& area, const PaintContext* ctx) override;

private:
    Orientation direction_ = Orientation::Horizontal;
    f32 spacing_ = 0.0f;
    FlexAlign main_align_ = FlexAlign::Start;
    FlexCrossAlign cross_align_ = FlexCrossAlign::Start;
};

}  // namespace yzk