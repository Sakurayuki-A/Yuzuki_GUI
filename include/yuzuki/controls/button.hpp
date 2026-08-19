#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

class Button : public Widget {
public:
    explicit Button(String text);

    const String& text() const { return text_; }
    void set_text(const String& text);

    bool pressed() const { return has_flag(Flag_Pressed); }
    bool hovered() const { return has_flag(Flag_Hovered); }

    void set_min_width(f32 width) { min_width_ = width; }
    f32 min_width() const { return min_width_; }

    void set_padding(f32 padding) { padding_ = padding; }
    f32 padding() const { return padding_; }

    void set_accent(bool accent) { accent_ = accent; }
    bool accent() const { return accent_; }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

    // Keyboard activation (Tab navigation + Enter)
    void activate() { on_click(); }

    virtual void on_click() {}

private:
    String text_;
    f32 min_width_ = 80.0f;
    f32 padding_ = 10.0f;
    bool accent_ = true;
};

}  // namespace yzk