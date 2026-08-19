#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

class ToggleSwitch : public Widget {
public:
    ToggleSwitch();

    bool checked() const { return checked_; }
    void set_checked(bool checked);

    virtual void on_toggled(bool checked) { (void)checked; }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

private:
    bool checked_ = false;
};

}  // namespace yzk
