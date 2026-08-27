#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

class ProgressBar : public Widget {
public:
    ProgressBar() = default;
    ~ProgressBar() override;

    f32 value() const { return value_; }
    ProgressBar& set_value(f32 value);

    bool indeterminate() const { return indeterminate_; }
    // True: registers a frame callback that sweeps the fill segment (the continuous
    // load README describes); False: stops it. Safe to toggle repeatedly.
    ProgressBar& set_indeterminate(bool indeterminate);

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;

private:
    f32 value_ = 0.0f;
    bool indeterminate_ = false;
    AnimationSystem::FrameToken anim_token_ = 0;  // live only while indeterminate
    f32 phase_ = 0.0f;                            // sweep position in [0,1)
};

}  // namespace yzk
