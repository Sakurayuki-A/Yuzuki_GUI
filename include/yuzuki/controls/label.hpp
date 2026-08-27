#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

namespace yzk {

// Theme text role: color resolved from Theme at paint time, follows theme switches
enum class TextRole { Primary, Secondary, Disabled };

class Label : public Widget {
public:
    explicit Label(String text);

    // text(String) is the fluent alias; keep both so old code keeps compiling.
    const String& text() const { return text_; }
    Label& set_text(const String& text);
    Label& text(String text) { return set_text(std::move(text)); }

    const Color& text_color() const { return text_color_; }
    Label& set_text_color(const Color& color);
    Label& text_color(const Color& color) { return set_text_color(color); }

    TextRole text_role() const { return text_role_; }
    Label& set_text_role(TextRole role);
    Label& text_role(TextRole role) { return set_text_role(role); }

    bool small() const { return small_; }
    Label& set_small(bool small);
    Label& small(bool small) { return set_small(small); }

    bool bold() const { return bold_; }
    Label& set_bold(bool bold);
    Label& bold(bool bold) { return set_bold(bold); }

    Label& set_align(TextAlignH align_h, TextAlignV align_v);
    Label& align(TextAlignH align_h, TextAlignV align_v) { return set_align(align_h, align_v); }

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