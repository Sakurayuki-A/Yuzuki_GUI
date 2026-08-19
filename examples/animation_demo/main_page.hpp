#pragma once
#include <yuzuki/yuzuki.hpp>
#include <yuzuki/ui/icon.hpp>

#include <string>
#include <vector>

using namespace yzk;

// Theme role colors: read from the active Theme, so they update on theme switch.
namespace demo {
inline Color WindowBg() { return Theme::get().background; }
inline Color CardBg() { return Theme::get().surface; }
inline Color CardBorder() { return Theme::get().border; }
inline Color HoverBg() { return Theme::get().surface_container_high; }
inline Color TrackBg() { return Theme::get().surface_container_low; }
inline Color Text() { return Theme::get().text; }
inline Color TextSecondary() { return Theme::get().text_secondary; }
inline Color Accent() { return Theme::get().accent; }
constexpr Color Green = Color::rgba(0x4ADE80FF);
constexpr Color Amber = Color::rgba(0xFBBF24FF);
constexpr Color Cyan = Color::rgba(0x22D3EEFF);
constexpr Color Pink = Color::rgba(0xF472B6FF);
}  // namespace demo

namespace yzk {

// Panel with a themed background; children fill it (local coords), optional fixed height.
class Panel : public Layout {
public:
    enum class Bg { Window, Card };
    explicit Panel(Bg bg, f32 fixed_height = 0.0f);

    Size measure_content(Size available, const PaintContext* ctx) override;
    void arrange_content(const RectF& area, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;

private:
    Bg bg_;
    f32 fixed_height_ = 0.0f;
};

// Easing gallery row: name on the left, track on the right; click to slide the dot.
class EasingRow : public Widget {
public:
    EasingRow(String name, Easing easing);

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;
    void run();

private:
    String name_;
    Easing easing_;
    f32 value_ = 0.0f;
};

// Color card: click cycles the palette with a smooth color transition.
class ColorCard : public Widget {
public:
    ColorCard();

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

private:
    size_t index_ = 0;
    Color color_;
};

// Expandable card: click toggles height with easing (mirrors codex's ToolCard).
class ExpandingCard : public Widget {
public:
    ExpandingCard(String title, String body);

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

private:
    String title_;
    String body_;
    bool expanded_ = false;
    f32 expand_progress_ = 0.0f;
    std::vector<String> lines_;
    f32 line_h_ = 18.0f;
};

Widget* make_animation_page(Window& win);

}  // namespace yzk