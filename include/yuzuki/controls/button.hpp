#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

#include <functional>

namespace yzk {

class Button : public Widget {
public:
    explicit Button(String text);

    const String& text() const { return text_; }
    Button& set_text(const String& text) {
        text_ = text;
        invalidate();
        return *this;
    }

    Button& set_min_width(f32 width) {
        min_width_ = width;
        invalidate();
        return *this;
    }
    f32 min_width() const { return min_width_; }

    Button& set_padding(f32 padding) {
        padding_ = padding;
        invalidate();
        return *this;
    }
    f32 padding() const { return padding_; }
    Button& padding(f32 padding) { return set_padding(padding); }

    Button& set_accent(bool accent) {
        accent_ = accent;
        invalidate();
        return *this;
    }
    bool accent() const { return accent_; }

    // Convenience callback registration; the virtual hook below is invoked with it.
    Button& set_on_click(std::function<void()> cb) {
        on_click_cb_ = std::move(cb);
        return *this;
    }
    // Fluent short name (Lego-style); same as set_on_click.
    Button& on_click(std::function<void()> cb) { return set_on_click(std::move(cb)); }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

    // Keyboard activation (Tab navigation + Enter)
    void activate() { on_click(); }

    virtual void on_click() {
        if (on_click_cb_) on_click_cb_();
    }

private:
    String text_;
    f32 min_width_ = 80.0f;
    f32 padding_ = 10.0f;
    bool accent_ = true;
    std::function<void()> on_click_cb_;
};

}  // namespace yzk