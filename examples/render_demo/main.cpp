#include <yuzuki/yuzuki.hpp>

#include <cmath>
#include <memory>
#include <string>

using namespace yzk;

namespace {

constexpr f32 kCardY = 566.0f;

struct Card : public Widget {
    Card(String title, f32 x, f32 y, f32 w, f32 h) : title(std::move(title)) {
        set_bounds(RectF::make(x, y, w, h));
        set_draggable(true);
        set_cursor(Cursor::Hand);
        set_focusable(false);
    }

    void set_fill(const Color& color) { fill_end_ = color; }
    void set_no_shadow(bool no_shadow) { no_shadow_ = no_shadow; }

    Size measure_impl(Size available, const PaintContext* ctx) override {
        (void)available;
        (void)ctx;
        return bounds_.size();
    }

    void paint_impl(PaintContext& ctx) override {
        const f32 ox = ctx.offset_x();
        const f32 oy = ctx.offset_y();
        ctx.set_offset(ox + bounds_.left, oy + bounds_.top);
        const RectF r = RectF::make(0.0f, 0.0f, bounds_.width(), bounds_.height());
        const Theme& theme = ctx.theme();
        const f32 blur = dragging_ ? 14.0f : hovered_ ? 14.0f : 6.0f;
        const Color shadow_color = theme.dark ? Color{0xFF, 0xFF, 0xFF, 200}
                                              : Color{0x00, 0x00, 0x00, 170};
        if (!no_shadow_) ctx.draw_shadow(r, 10.0f, blur, shadow_color);
        const Color end = fill_end_.is_transparent() ? Color{0xE8, 0xE8, 0xFF} : fill_end_;
        ctx.fill_gradient(r, theme.surface, end, true, 10.0f);
        ctx.draw_border(r, dragging_ ? theme.accent : hovered_ ? theme.accent : theme.border,
                        1.0f, 10.0f);
        ctx.draw_text(title, r, theme.text, TextAlignH::Center, TextAlignV::Center);
        ctx.set_offset(ox, oy);
    }

    void on_event(Event& e) override {
        if (e.type == EventType::MouseEnter) {
            hovered_ = true;
            invalidate();
        } else if (e.type == EventType::MouseLeave) {
            hovered_ = false;
            invalidate();
        } else if (e.type == EventType::DragStart) {
            dragging_ = true;
            invalidate();
        } else if (e.type == EventType::DragEnd) {
            dragging_ = false;
            hovered_ = false;
            invalidate();
        }
    }

    String title;
    Color fill_end_;
    bool hovered_ = false;
    bool dragging_ = false;
    bool no_shadow_ = false;
};

class RenderDemo : public Widget {
public:
    RenderDemo() {
        set_focusable(false);
        big_ = new Card("Big", 60.0f, kCardY - 10.0f, 200.0f, 130.0f);
        big_->set_fill(Color{0xFF, 0x8C, 0x00});
        append_child(big_);
        card0_ = new Card("Gradient", 16.0f, kCardY, 150.0f, 110.0f);
        card0_->set_fill(Color{0xFF, 0xFF, 0xFF});
        card0_->set_no_shadow(true);
        card1_ = new Card("Circle", 173.0f, kCardY, 150.0f, 110.0f);
        card1_->set_fill(Color{0x00, 0xB8, 0xA3});
        card2_ = new Card("Shadow", 330.0f, kCardY, 150.0f, 110.0f);
        card2_->set_fill(Color{0xE9, 0x1E, 0x63});
        append_child(card0_);
        append_child(card1_);
        append_child(card2_);
    }

    ~RenderDemo() override {
        delete big_;
        delete card0_;
        delete card1_;
        delete card2_;
    }

    Size measure_impl(Size available, const PaintContext* ctx) override {
        (void)ctx;
        return Size{available.width > 0.0f ? available.width : 0.0f, 690.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        const Theme& theme = ctx.theme();
        const RectF& b = bounds_;

        const f32 ox = ctx.offset_x();
        const f32 oy = ctx.offset_y();
        ctx.set_offset(ox + b.left, oy + b.top);

        const Color palette[6] = {
            Color{0x67, 0x50, 0xA4},
            Color{0x29, 0x79, 0xFF},
            Color{0x00, 0xB8, 0xA3},
            Color{0xFF, 0xB3, 0x00},
            Color{0xF4, 0x51, 0x1E},
            Color{0xE9, 0x1E, 0x63},
        };

        draw_section_label(ctx, theme, "Linear gradients", 0.0f);
        const f32 bar_w = (b.width() - 32.0f - 5.0f * 4.0f) / 6.0f;
        for (int i = 0; i < 6; ++i) {
            const f32 x = 16.0f + i * (bar_w + 4.0f);
            ctx.fill_gradient(RectF::make(x, 22.0f, bar_w, 34.0f), palette[i],
                              Color{0x1E, 0x1E, 0x1E}, true);
        }

        draw_section_label(ctx, theme, "Radial & Sweep", 70.0f);
        const f32 radial_r = 80.0f;
        const RectF radial_rect = RectF::make(16.0f, 92.0f, radial_r * 2.0f, radial_r * 2.0f);
        ctx.fill_radial_gradient(Point{radial_rect.left + radial_r, radial_rect.top + radial_r},
                                 radial_r, palette[0], palette[5]);
        ctx.draw_border(radial_rect, theme.border, 1.0f, radial_r);
        const RectF sweep_rect = RectF::make(b.width() - 176.0f, 92.0f, 160.0f, 160.0f);
        ctx.fill_sweep_gradient(sweep_rect,
                                Point{sweep_rect.left + sweep_rect.width() * 0.5f,
                                      sweep_rect.top + sweep_rect.height() * 0.5f},
                                -1.5707963f, 6.2831853f, palette[0], palette[2], 4.0f);
        ctx.draw_border(sweep_rect, theme.border, 1.0f, 4.0f);

        draw_section_label(ctx, theme, "Circles with shadows", 268.0f);
        ctx.draw_text_small("Shadow blur falloff", RectF::make(180.0f, 270.0f, 200.0f, 18.0f),
                            theme.text_secondary);
        const Color circle_shadow = theme.dark ? Color{0xFF, 0xFF, 0xFF, 180}
                                               : Color{0x00, 0x00, 0x00, 150};
        for (int i = 0; i < 3; ++i) {
            const f32 cx = 218.0f + i * 72.0f;
            const f32 cy = 340.0f;
            const f32 r = 20.0f - static_cast<f32>(i) * 4.0f;
            ctx.draw_shadow(RectF::make(cx - r, cy - r, r * 2.0f, r * 2.0f), r, 8.0f,
                            circle_shadow);
            ctx.fill_circle(Point{cx, cy}, r, theme.accent.with_alpha(200));
        }

        const Color falloff_shadow = theme.dark ? Color{0xFF, 0xFF, 0xFF, 200}
                                                : Color{0x00, 0x00, 0x00, 160};
        const f32 blur_offsets[4] = {2.0f, 5.0f, 10.0f, 12.0f};
        for (int i = 0; i < 4; ++i) {
            const f32 x = 16.0f + i * 40.0f;
            ctx.draw_shadow(RectF::make(x, 326.0f, 28.0f, 28.0f), 6.0f, blur_offsets[i],
                            falloff_shadow);
            ctx.fill_rounded(RectF::make(x, 326.0f, 28.0f, 28.0f), theme.accent, 6.0f);
        }

        // Cards are painted first (bottom layer), the backdrop-blur panel last (top layer),
        // so cards dragged into the panel are covered by and shown through the blur
        draw_section_label(ctx, theme, "Interactive cards (draggable)", 544.0f);

        for (Widget* child = first_child(); child; child = child->next_sibling()) {
            if (child->visible()) child->paint(ctx);
        }

        draw_section_label(ctx, theme, "Backdrop blur", 420.0f);
        const f32 blur_w = (b.width() - 32.0f - 3.0f * 4.0f) / 4.0f;
        for (int i = 0; i < 2; ++i) {
            const f32 x = 16.0f + i * (blur_w + 4.0f);
            ctx.fill_gradient(RectF::make(x, 442.0f, blur_w, 34.0f), palette[i * 2],
                              palette[i * 2 + 1], true);
            ctx.fill_circle(Point{x + blur_w * 0.5f, 446.0f}, 10.0f,
                            palette[(i + 2) % 6].with_alpha(200));
        }
        const f32 panel_w = b.width() - 192.0f;
        const RectF panel = RectF::make((b.width() - panel_w) * 0.5f, 444.0f, panel_w, 44.0f);
        ctx.draw_backdrop_blur(panel, 16.0f, Color{0xFF, 0xFF, 0xFF, 60}, 10.0f);
        ctx.draw_border(panel, Color{0xFF, 0xFF, 0xFF, 120}, 1.0f, 10.0f);
        ctx.draw_text("BackdropBlur", RectF::make(panel.left, panel.top, panel.width(), panel.height()),
                      Color{0x1E, 0x1E, 0x1E, 230}, TextAlignH::Center, TextAlignV::Center);

        // Overlap highlight: outline the card and the blur panel when they intersect
        {
            bool overlapped = false;
            for (Widget* child = first_child(); child; child = child->next_sibling()) {
                if (!child->visible()) continue;
                if (child->bounds().intersects(panel)) {
                    overlapped = true;
                    ctx.draw_border(child->bounds(), Color{0x00, 0xD4, 0xFF, 230}, 2.0f, 10.0f);
                }
            }
            if (overlapped) {
                ctx.draw_border(panel, Color{0x00, 0xD4, 0xFF, 230}, 2.0f, 10.0f);
            }
        }

        ctx.set_offset(ox, oy);
    }

private:
    void draw_section_label(PaintContext& ctx, const Theme& theme, const String& text, f32 y) {
        ctx.draw_text_small(text, RectF::make(16.0f, y + 2.0f, bounds_.width() - 32.0f, 18.0f),
                            theme.text_secondary);
    }

    Card* big_ = nullptr;
    Card* card0_ = nullptr;
    Card* card1_ = nullptr;
    Card* card2_ = nullptr;
};

class ThemeButton : public Button {
public:
    ThemeButton() : Button("Toggle Theme") {
        set_accent(false);
    }

    void on_click() override {
        Theme::set(Theme::get().dark ? Theme::make_light() : Theme::make_dark());
        window()->invalidate_all();
    }
};

}  // namespace

int main() {
    Application& app = Application::instance();

    Window window("YuzukiUI Render Demo", 520, 760);
    if (!window.create()) return 1;

    {
        wchar_t exe_path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        const std::wstring dir(exe_path, wcsrchr(exe_path, L'\\') + 1);
        window.backend().add_font_file(utf::to_utf8(dir + L"Satoshi-Regular.otf"));
    }

    auto root = std::make_unique<Widget>();
    root->set_focusable(false);

    auto title = std::make_unique<Label>("YuzukiUI Render Primitives");
    title->set_bold(true);
    title->set_bounds(RectF::make(16.0f, 14.0f, 488.0f, 30.0f));

    auto theme_button = std::make_unique<ThemeButton>();
    theme_button->set_bounds(RectF::make(376.0f, 14.0f, 128.0f, 30.0f));

    auto demo = std::make_unique<RenderDemo>();
    demo->set_bounds(RectF::make(16.0f, 52.0f, 488.0f, 690.0f));

    root->append_child(title.get());
    root->append_child(theme_button.get());
    root->append_child(demo.get());

    window.set_root(root.get());
    window.show();

    const int exit_code = app.run();

    window.set_root(nullptr);
    return exit_code;
}
