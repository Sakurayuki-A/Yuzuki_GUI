#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>
#include <yuzuki/ui/icon.hpp>

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

    // Optional icon glyph drawn before the text (via the global Icon Provider).
    Button& set_icon(IconId icon) {
        icon_ = icon;
        invalidate();
        return *this;
    }
    IconId icon() const { return icon_; }
    Button& icon(IconId icon) { return set_icon(icon); }

    Button& set_icon_size(f32 size) {
        icon_size_ = size;
        invalidate();
        return *this;
    }
    f32 icon_size() const { return icon_size_; }

    // Fluent chaining alias that keeps the Button& return type (Widget::set_min_width
    // returns Widget&, which would break Button-specific fluent chains). Storage is
    // Widget::min_size_ — the same concept, one data source.
    Button& set_min_width(f32 width) { return static_cast<Button&>(Widget::set_min_width(width)); }
    f32 min_width() const { return min_size().width; }
    Button& width(f32 w) { return set_min_width(w); }

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

    String uia_name() const override { return text_; }
    String uia_role() const override { return "Button"; }

    // Keyboard activation (Tab navigation + Enter)
    void activate() { on_click(); }

    virtual void on_click() {
        if (on_click_cb_) on_click_cb_();
    }

private:
    String text_;
    f32 padding_ = 10.0f;
    bool accent_ = true;
    IconId icon_ = IconId::None;
    f32 icon_size_ = 16.0f;
    std::function<void()> on_click_cb_;
};

}  // namespace yzk