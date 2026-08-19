#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

class ProgressBar : public Widget {
public:
    ProgressBar() = default;

    f32 value() const { return value_; }
    void set_value(f32 value);

    bool indeterminate() const { return indeterminate_; }
    void set_indeterminate(bool indeterminate);

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;

private:
    f32 value_ = 0.0f;
    bool indeterminate_ = false;
};

}  // namespace yzk
