#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

#include <functional>

namespace yzk {

class Slider : public Widget {
public:
    Slider();

    f32 value() const { return value_; }
    f32 min() const { return min_; }
    f32 max() const { return max_; }

    Slider& set_range(f32 min, f32 max);
    Slider& set_value(f32 value);

    // Convenience callback registration; the virtual hook below is invoked with it.
    Slider& set_on_changed(std::function<void(f32)> cb) {
        on_changed_cb_ = std::move(cb);
        return *this;
    }
    Slider& on_changed(std::function<void(f32)> cb) { return set_on_changed(std::move(cb)); }

    virtual void on_changed(f32 value) {
        if (on_changed_cb_) on_changed_cb_(value);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

private:
    f32 value_at_x(f32 x) const;
    // Window-client X -> widget-local X (mouse events arrive in window coordinates,
    // while bounds_ is parent-local; skipping this conversion offsets the thumb by
    // the accumulated ancestor offset — and any layout shift mid-drag desyncs it).
    f32 local_x(f32 window_x) const;

    f32 min_ = 0.0f;
    f32 max_ = 100.0f;
    f32 value_ = 0.0f;
    bool dragging_ = false;
    std::function<void(f32)> on_changed_cb_;
};

}  // namespace yzk
