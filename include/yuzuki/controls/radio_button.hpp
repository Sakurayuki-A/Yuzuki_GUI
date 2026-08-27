#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

#include <functional>

namespace yzk {

class RadioButton : public Widget {
public:
    explicit RadioButton(String text = String());

    const String& text() const { return text_; }
    RadioButton& set_text(const String& text);
    RadioButton& text(String text) { return set_text(std::move(text)); }

    bool checked() const { return checked_; }
    RadioButton& set_checked(bool checked);
    RadioButton& checked(bool checked) { return set_checked(checked); }

    // Convenience callback registration; the virtual hook below is invoked with it.
    RadioButton& set_on_toggled(std::function<void(bool)> cb) {
        on_toggled_cb_ = std::move(cb);
        return *this;
    }
    RadioButton& on_toggled(std::function<void(bool)> cb) { return set_on_toggled(std::move(cb)); }

    virtual void on_toggled(bool checked) {
        if (on_toggled_cb_) on_toggled_cb_(checked);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

private:
    void check_siblings();

    String text_;
    bool checked_ = false;
    std::function<void(bool)> on_toggled_cb_;
};

}  // namespace yzk
