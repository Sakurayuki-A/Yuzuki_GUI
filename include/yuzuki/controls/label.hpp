#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/paint.hpp>

#include <functional>
#include <vector>

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
    TextAlignH align_h() const { return align_h_; }
    TextAlignV align_v() const { return align_v_; }

    // ===== RichText lite (5.3.2) =====
    // One string with inline spans, drawn left-to-right. Markup (non-nested):
    //   **bold**, ~highlight~ (accent color), [label](action) — the link is
    //   clickable and fires on_span_click with the action string.
    Label& set_rich_text(bool rich);
    bool rich_text() const { return rich_; }
    Label& set_on_span_click(std::function<void(const String& action)> cb);
    Label& on_span_click(std::function<void(const String& action)> cb) {
        return set_on_span_click(std::move(cb));
    }
    // Fires the callback of the link span at widget-local (x, y); true if any.
    bool hit_link(f32 x, f32 y);

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

    String uia_name() const override { return text_; }
    String uia_role() const override { return "Text"; }

private:
    // One visible run; x/width/top/height filled in at paint time (widget-local).
    struct Run {
        String str;
        u16 weight = 400;
        bool link = false;
        bool highlight = false;
        String action;
        f32 x = 0.0f;
        f32 width = 0.0f;
        f32 top = 0.0f;
        f32 height = 0.0f;
    };
    void parse_rich();
    void relayout_runs(PaintContext& ctx);
    u32 run_at(f32 local_x, f32 local_y) const;

    String text_;
    Color text_color_{0, 0, 0, 0};
    TextRole text_role_ = TextRole::Primary;
    bool small_ = false;
    bool bold_ = false;
    TextAlignH align_h_ = TextAlignH::Center;
    TextAlignV align_v_ = TextAlignV::Center;

    bool rich_ = false;
    std::vector<Run> runs_;
    std::function<void(const String& action)> on_span_click_cb_;
    bool hovered_link_ = false;
    bool geometry_valid_ = false;
};

}  // namespace yzk