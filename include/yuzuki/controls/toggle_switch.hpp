#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

#include <functional>

namespace yzk {

class ToggleSwitch : public Widget {
public:
    ToggleSwitch();

    bool checked() const { return checked_; }
    ToggleSwitch& set_checked(bool checked);
    ToggleSwitch& checked(bool checked) { return set_checked(checked); }

    // Convenience callback registration; the virtual hook below is invoked with it.
    ToggleSwitch& set_on_toggled(std::function<void(bool)> cb) {
        on_toggled_cb_ = std::move(cb);
        return *this;
    }
    ToggleSwitch& on_toggled(std::function<void(bool)> cb) { return set_on_toggled(std::move(cb)); }

    virtual void on_toggled(bool checked) {
        if (on_toggled_cb_) on_toggled_cb_(checked);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

private:
    bool checked_ = false;
    std::function<void(bool)> on_toggled_cb_;
};

}  // namespace yzk
