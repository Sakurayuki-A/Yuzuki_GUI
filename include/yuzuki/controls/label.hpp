#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

// Theme text role: color resolved from Theme at paint time, follows theme switches
enum class TextRole { Primary, Secondary, Disabled };

class Label : public Widget {
public:
    explicit Label(String text);

    const String& text() const { return text_; }
    void set_text(const String& text);

    const Color& text_color() const { return text_color_; }
    void set_text_color(const Color& color);

    TextRole text_role() const { return text_role_; }
    void set_text_role(TextRole role);

    bool small() const { return small_; }
    void set_small(bool small);

    bool bold() const { return bold_; }
    void set_bold(bool bold);

    void set_align(TextAlignH align_h, TextAlignV align_v);

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;

private:
    String text_;
    Color text_color_{0, 0, 0, 0};
    TextRole text_role_ = TextRole::Primary;
    bool small_ = false;
    bool bold_ = false;
    TextAlignH align_h_ = TextAlignH::Center;
    TextAlignV align_v_ = TextAlignV::Center;
};

}  // namespace yzk