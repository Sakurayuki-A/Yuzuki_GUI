#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

#include <functional>

namespace yzk {

class CheckBox : public Widget {
public:
    explicit CheckBox(String text = String());

    const String& text() const { return text_; }
    CheckBox& set_text(const String& text);
    CheckBox& text(String text) { return set_text(std::move(text)); }

    bool checked() const { return checked_; }
    CheckBox& set_checked(bool checked);
    CheckBox& checked(bool checked) { return set_checked(checked); }

    // Convenience callback registration; the virtual hook below is invoked with it.
    CheckBox& set_on_toggled(std::function<void(bool)> cb) {
        on_toggled_cb_ = std::move(cb);
        return *this;
    }
    CheckBox& on_toggled(std::function<void(bool)> cb) { return set_on_toggled(std::move(cb)); }

    virtual void on_toggled(bool checked) {
        if (on_toggled_cb_) on_toggled_cb_(checked);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

    String uia_name() const override { return text_; }
    String uia_role() const override { return "CheckBox"; }

private:
    String text_;
    bool checked_ = false;
    std::function<void(bool)> on_toggled_cb_;
};

}  // namespace yzk
