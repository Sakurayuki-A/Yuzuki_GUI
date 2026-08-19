#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

class Slider : public Widget {
public:
    Slider();

    f32 value() const { return value_; }
    f32 min() const { return min_; }
    f32 max() const { return max_; }

    void set_range(f32 min, f32 max);
    void set_value(f32 value);

    virtual void on_changed(f32 value) { (void)value; }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

private:
    f32 value_at_x(f32 x) const;

    f32 min_ = 0.0f;
    f32 max_ = 100.0f;
    f32 value_ = 0.0f;
    bool dragging_ = false;
};

}  // namespace yzk
