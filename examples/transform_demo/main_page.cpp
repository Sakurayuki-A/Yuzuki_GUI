#include "main_page.hpp"

#include <chrono>
#include <cstdio>

using namespace yzk;

namespace demo {

Color bg() { return Theme::get().background; }
Color card_bg() { return Theme::get().surface; }
Color text() { return Theme::get().text; }
Color text_secondary() { return Theme::get().text_secondary; }
Color accent() { return Theme::get().accent; }
Color green() { return Color{0x3b, 0xc9, 0x8c, 255}; }
Color amber() { return Color{0xf2, 0xa8, 0x3a, 255}; }
Color cyan() { return Color{0x35, 0xbd, 0xe8, 255}; }
Color pink() { return Color{0xec, 0x68, 0x9a, 255}; }
Color red() { return Color{0xe5, 0x5d, 0x5d, 255}; }

}  // namespace demo

namespace {

// Demo cards mutate only visual state (set_scale / set_rotate_deg / set_opacity /
// set_translate); implicit transitions tween the setters automatically. Hit-testing
// still uses the layout rect, so transforms are purely visual, like CSS transform.
enum class TfxMode { ScaleHover, SpinClick, FadeClick };

class TfxCard : public Widget {
public:
    TfxCard(String label, Color color, TfxMode mode)
        : label_(std::move(label)), color_(color), mode_(mode) {
        set_cursor(Cursor::Hand);
        if (mode_ == TfxMode::ScaleHover) set_transition(180.0f);
        if (mode_ == TfxMode::FadeClick) set_transition(260.0f);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override {
        (void)available;
        (void)ctx;
        return Size{96.0f, 96.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        const RectF b = bounds_;
        ctx.fill_rounded(b, color_, 12.0f);
        ctx.draw_text_small(label_, b, demo::text(), TextAlignH::Center, TextAlignV::Center);
    }

    void on_event(Event& e) override {
        switch (e.type) {
            case EventType::MouseEnter:
                add_flag(Flag_Hovered);
                if (mode_ == TfxMode::ScaleHover) {
                    set_scale(1.08f, 1.08f);  // Implicit transition: auto-tween
                }
                e.consumed = true;
                break;

            case EventType::MouseLeave:
                remove_flag(Flag_Hovered);
                if (mode_ == TfxMode::ScaleHover) {
                    set_scale(1.0f, 1.0f);  // Implicit transition: auto-tween back
                }
                e.consumed = true;
                break;

            case EventType::MouseDown:
                if ((e.data.mouse.buttons & MouseButton_Left) != 0) {
                    if (mode_ == TfxMode::SpinClick) {
                        AnimationSystem& as = AnimationSystem::instance();
                        // Finish the old tween first (it snaps to its end at a 360° multiple),
                        // or concurrent tweens would fight over the angle and skew the end state
                        if (spin_token_) as.finish_tween(spin_token_);
                        spin_token_ = as.tween(rotate_deg(), rotate_deg() + 360.0f, 700.0f,
                                               Easing::OutBack,
                                               [this](f32 v) { set_rotate_deg(v); });
                    } else if (mode_ == TfxMode::FadeClick) {
                        // Implicit transition: opacity + translate tween together
                        if (faded_) {
                            set_opacity(1.0f);
                            set_translate(0.0f, 0.0f);
                        } else {
                            set_opacity(0.15f);
                            set_translate(0.0f, 6.0f);
                        }
                        faded_ = !faded_;
                    }
                    e.consumed = true;
                }
                break;

            default:
                break;
        }
    }

private:
    String label_;
    Color color_;
    TfxMode mode_;
    AnimationSystem::Token spin_token_ = 0;
    bool faded_ = false;
};

// Frame-driven auto animations (spin / bounce / pulse opacity). on_frame callbacks follow
// the render pace, unlike WM_TIMER, which lags and coalesces under mouse-message floods
class AutoCard : public Widget {
public:
    AutoCard(String label, Color color) : label_(std::move(label)), color_(color) {}

    Size measure_impl(Size available, const PaintContext* ctx) override {
        (void)available;
        (void)ctx;
        return Size{96.0f, 96.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        const RectF b = bounds_;
        ctx.fill_rounded(b, color_, 12.0f);
        ctx.draw_text_small(label_, b, demo::text(), TextAlignH::Center, TextAlignV::Center);
    }

private:
    String label_;
    Color color_;
};

class AutoShowcase : public Widget {
public:
    AutoShowcase() {
        spin_ = new AutoCard("rotate", demo::cyan());
        bounce_ = new AutoCard("bounce", demo::amber());
        pulse_ = new AutoCard("pulse", demo::pink());
        append_child(spin_);
        append_child(bounce_);
        append_child(pulse_);
        const f32 kStartMs = static_cast<f32>(
            std::chrono::duration<double, std::milli>(kStart.time_since_epoch()).count());
        frame_token_ = AnimationSystem::instance().on_frame([this, kStartMs](f32 now_ms) {
            const f32 t_ms = now_ms - kStartMs;
            spin_->set_rotate_deg(t_ms * 0.5f / 16.0f);
            bounce_->set_translate(0.0f, std::sin(t_ms * 0.004f) * 14.0f);
            pulse_->set_opacity(0.75f + 0.25f * (0.5f + 0.5f * std::sin(t_ms * 0.003f)));
        });
    }

    ~AutoShowcase() override {
        if (frame_token_) AnimationSystem::instance().stop_frame(frame_token_);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override {
        (void)available;
        (void)ctx;
        return Size{0.0f, 96.0f};
    }

    void perform_layout(const PaintContext* ctx) override {
        Widget* child = first_child();
        f32 x = 0.0f;
        while (child) {
            child->set_bounds(RectF::make(x, 0.0f, 96.0f, 96.0f));
            child->perform_layout(ctx);
            x += 96.0f + 14.0f;
            child = child->next_sibling();
        }
    }

private:
    static const std::chrono::steady_clock::time_point kStart;
    AnimationSystem::FrameToken frame_token_ = 0;
    AutoCard* spin_ = nullptr;
    AutoCard* bounce_ = nullptr;
    AutoCard* pulse_ = nullptr;
};

const std::chrono::steady_clock::time_point AutoShowcase::kStart =
    std::chrono::steady_clock::now();

// Top bar: background fill + child content (no rounded corners)
class TopBar : public Widget {
public:
    Size measure_impl(Size available, const PaintContext* ctx) override {
        (void)available;
        (void)ctx;
        return Size{0.0f, 48.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        ctx.fill_rect(bounds_, demo::bg());
        Widget::paint_impl(ctx);
    }
};

}  // namespace

// ===== Page =====

Widget* make_transform_page(Window& win) {
    auto root = new DockPanel;

    auto topbar = new TopBar;
    root->dock(topbar, Dock::Top);
    auto top_row = new DockPanel;
    top_row->set_padding(14.0f);
    topbar->append_child(top_row);
    auto title = new Label("Visual Transform Foundation");
    title->set_bold(true);
    title->set_align(TextAlignH::Left, TextAlignV::Center);
    top_row->dock(title, Dock::Left);

    auto body = new FlexBox(Orientation::Vertical);
    body->set_padding(18.0f);
    body->set_spacing(14.0f);
    // Stretch on the cross axis so rows fill the container width; with the default
    // Start, rows stay content-width and grow has no leftover space to distribute
    body->set_align_cross(FlexCrossAlign::Stretch);
    root->dock(body, Dock::Fill);

    auto add_section = [&](const char* hint, std::initializer_list<Color> colors, TfxMode mode) {
        auto label = new Label(hint);
        label->set_text_role(TextRole::Secondary);
        label->set_small(true);
        label->set_align(TextAlignH::Left, TextAlignV::Center);
        body->append_child(label);

        auto row = new FlexBox;
        row->set_spacing(14.0f);
        int idx = 0;
        for (const Color& c : colors) {
            char name[16];
            std::snprintf(name, sizeof(name), "card %d", idx + 1);
            auto card = new TfxCard(name, c, mode);
            row->append_child(card);
            ++idx;
        }
        body->append_child(row);
    };

    add_section("Hover: scale 1.08 - visual state, zero paint code",
                {demo::accent(), demo::green(), demo::amber()}, TfxMode::ScaleHover);
    add_section("Click: rotate 360 deg - OutBack easing",
                {demo::cyan(), demo::pink(), demo::red()}, TfxMode::SpinClick);
    add_section("Click: fade + slide - opacity & translate",
                {demo::green(), demo::accent(), demo::amber()}, TfxMode::FadeClick);

    auto auto_hint = new Label("frame-driven visual states: rotate / bounce / pulse");
    auto_hint->set_text_role(TextRole::Secondary);
    auto_hint->set_small(true);
    auto_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    body->append_child(auto_hint);

    auto showcase = new AutoShowcase;
    body->append_child(showcase);

    // FlexBox demo: fixed / grow 1 / grow 2 — leftover width split by grow ratio
    auto flex_hint = new Label("FlexBox: fixed / grow 1 / grow 2 - remaining width split by grow");
    flex_hint->set_text_role(TextRole::Secondary);
    flex_hint->set_small(true);
    flex_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    body->append_child(flex_hint);

    auto flex_row = new FlexBox;
    flex_row->set_spacing(14.0f);
    auto flex_fixed = new TfxCard("fixed", demo::cyan(), TfxMode::ScaleHover);
    auto flex_g1 = new TfxCard("grow 1", demo::green(), TfxMode::ScaleHover);
    auto flex_g2 = new TfxCard("grow 2", demo::accent(), TfxMode::ScaleHover);
    flex_g1->set_flex_grow(1.0f);
    flex_g2->set_flex_grow(2.0f);
    flex_row->append_child(flex_fixed);
    flex_row->append_child(flex_g1);
    flex_row->append_child(flex_g2);
    body->append_child(flex_row);

    // Box demo: rounded bg / border / shadow + padding
    auto box_hint = new Label("Box: bg+radius / border / shadow - padding wraps content");
    box_hint->set_text_role(TextRole::Secondary);
    box_hint->set_small(true);
    box_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    body->append_child(box_hint);

    auto box_row = new FlexBox;
    box_row->set_spacing(14.0f);
    auto box_flat = new Box;
    box_flat->set_bg(demo::accent());
    box_flat->set_radius(12.0f);
    box_flat->set_padding(14.0f);
    auto box_flat_label = new Label("rounded bg");
    box_flat_label->set_text_color(Color{0xff, 0xff, 0xff, 0xff});
    box_flat->append_child(box_flat_label);
    box_row->append_child(box_flat);

    auto box_border = new Box;
    box_border->set_padding(14.0f);
    box_border->set_radius(12.0f);
    box_border->set_border(1.5f, demo::accent());
    auto box_border_label = new Label("border card");
    box_border->append_child(box_border_label);
    box_row->append_child(box_border);

    auto box_shadow = new Box;
    box_shadow->set_bg(Color{0xff, 0xff, 0xff, 0xff});
    box_shadow->set_radius(12.0f);
    box_shadow->set_padding(14.0f);
    box_shadow->set_shadow(14.0f, 5.0f);
    box_shadow->set_shadow_color(Color{0xff, 0xff, 0xff, 55});  // Black shadows are invisible on dark, so use a light glow
    auto box_shadow_label = new Label("shadow card");
    box_shadow_label->set_text_color(Color{0x1f, 0x1f, 0x1f, 0xff});
    box_shadow->append_child(box_shadow_label);
    box_row->append_child(box_shadow);
    body->append_child(box_row);

    return root;
}