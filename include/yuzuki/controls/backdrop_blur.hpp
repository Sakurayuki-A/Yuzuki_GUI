#pragma once
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/widget.hpp>

namespace yzk {

class BackdropBlur : public Widget {
public:
    BackdropBlur() = default;

    f32 blur() const { return blur_; }
    void set_blur(f32 blur);

    const Color& tint() const { return tint_; }
    void set_tint(const Color& tint);

    f32 corner_radius() const { return corner_radius_; }
    void set_corner_radius(f32 radius);

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;

private:
    f32 blur_ = 12.0f;
    Color tint_{0xE8, 0xE8, 0xE8, 180};
    f32 corner_radius_ = 10.0f;
};

}  // namespace yzk