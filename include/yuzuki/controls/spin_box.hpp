#pragma once
#include <yuzuki/controls/text_box.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

class SpinBox : public TextBox {
public:
    SpinBox();
    SpinBox(f64 value, f64 min, f64 max, f64 step);

    f64 value() const { return value_; }
    void set_value(f64 value);

    void set_range(f64 min, f64 max);
    f64 minimum() const { return min_; }
    f64 maximum() const { return max_; }

    void set_step(f64 step) { step_ = step; }
    f64 step() const { return step_; }

    void set_decimals(i32 decimals) {
        decimals_ = decimals;
        sync_text();
    }
    i32 decimals() const { return decimals_; }

    void set_spin_width(f32 width) { spin_width_ = width; }
    f32 spin_width() const { return spin_width_; }

    virtual void on_value_changed(f64 value) { (void)value; }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

private:
    void step_by(i32 dir);
    f64 parse_value() const;
    void sync_text();

    f64 value_ = 0.0;
    f64 min_ = 0.0;
    f64 max_ = 100.0;
    f64 step_ = 1.0;
    i32 decimals_ = 0;
    f32 spin_width_ = 26.0f;
    bool hover_up_ = false;
    bool hover_down_ = false;
};

}  // namespace yzk