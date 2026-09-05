#pragma once
#include <yuzuki/controls/text_box.hpp>
#include <yuzuki/ui/paint.hpp>

#include <functional>

namespace yzk {

class SpinBox : public TextBox {
public:
    SpinBox();
    SpinBox(f64 value, f64 min, f64 max, f64 step);

    f64 value() const { return value_; }
    SpinBox& set_value(f64 value);

    SpinBox& set_range(f64 min, f64 max);
    f64 min() const { return min_; }
    f64 max() const { return max_; }

    SpinBox& set_step(f64 step) {
        step_ = step;
        return *this;
    }
    f64 step() const { return step_; }

    SpinBox& set_decimals(i32 decimals) {
        decimals_ = decimals;
        sync_text();
        return *this;
    }
    i32 decimals() const { return decimals_; }

    SpinBox& set_spin_width(f32 width) {
        spin_width_ = width;
        return *this;
    }
    f32 spin_width() const { return spin_width_; }

    // Convenience callback registration; the virtual hook below is invoked with it.
    SpinBox& set_on_changed(std::function<void(f64)> cb) {
        on_changed_cb_ = std::move(cb);
        return *this;
    }
    SpinBox& on_changed(std::function<void(f64)> cb) { return set_on_changed(std::move(cb)); }

    virtual void on_changed(f64 value) {
        if (on_changed_cb_) on_changed_cb_(value);
    }

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
    std::function<void(f64)> on_changed_cb_;
};

}  // namespace yzk