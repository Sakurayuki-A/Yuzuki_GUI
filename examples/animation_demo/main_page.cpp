#include "main_page.hpp"

#include <yuzuki/controls/notification.hpp>
#include <yuzuki/controls/context_menu.hpp>

#include <cstdlib>

using namespace yzk;

namespace {

// Toggles between dark and light themes.
class ToggleThemeButton : public Button {
public:
    explicit ToggleThemeButton(Window& win) : Button("Toggle Theme"), win_(win) {}

    void on_click() override {
        dark_ = !dark_;
        Theme::set(dark_ ? Theme::make_dark() : Theme::make_light());
        win_.invalidate_all();
    }

private:
    Window& win_;
    bool dark_ = true;
};

// Right-click host: opens a context menu; menu items show notifications.
class MenuHost : public Widget {
public:
    explicit MenuHost(Window& win) : win_(win) {
        menu_.add_item("Copy text", [this] { notify("Copied to clipboard"); });
        menu_.add_item("Run animation", [this] { notify("Animation ran"); });
        menu_.add_separator();
        menu_.add_item("About", [this] { notify("ContextMenu demo"); });
    }

    Size measure_impl(Size available, const PaintContext* ctx) override {
        (void)available;
        (void)ctx;
        return Size{0.0f, 44.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        const RectF b = bounds_;
        ctx.fill_rounded(b, demo::CardBg(), 8.0f);
        ctx.draw_border(b, demo::CardBorder(), 1.0f, 8.0f);
        ctx.draw_text("Right-click me (ContextMenu)", b, demo::TextSecondary());
        ctx.draw_text_small("Esc or click outside to dismiss",
                            RectF::make(b.left, b.bottom - 20.0f, b.width(), 16.0f),
                            demo::TextSecondary().with_alpha(140), TextAlignH::Center,
                            TextAlignV::Bottom);
    }

    void on_event(Event& e) override {
        if (e.type == EventType::MouseDown && (e.data.mouse.buttons & MouseButton_Right) != 0) {
            menu_.open(win_, e.data.mouse.x, e.data.mouse.y);
            e.consumed = true;
            return;
        }
        Widget::on_event(e);
    }

private:
    void notify(const String& text) {
        NotificationManager::instance().show(win_, "ContextMenu", text, NotificationType::Info);
    }

    Window& win_;
    ContextMenu menu_;
};

Color lerp_color(Color a, Color b, f32 t) {
    return Color{
        static_cast<u8>(a.r + (b.r - a.r) * t),
        static_cast<u8>(a.g + (b.g - a.g) * t),
        static_cast<u8>(a.b + (b.b - a.b) * t),
        static_cast<u8>(a.a + (b.a - a.a) * t),
    };
}

const Color kPalette[] = {demo::Accent(), demo::Green, demo::Amber, demo::Cyan, demo::Pink};
constexpr const char* kPaletteNames[] = {"Indigo", "Green", "Amber", "Cyan", "Pink"};
constexpr size_t kPaletteCount = sizeof(kPalette) / sizeof(kPalette[0]);

class RunAllButton : public Button {
public:
    explicit RunAllButton(std::vector<EasingRow*> rows) : Button("Run All"), rows_(std::move(rows)) {}
    void on_click() override {
        for (EasingRow* row : rows_) row->run();
    }

private:
    std::vector<EasingRow*> rows_;
};

class RandomizeButton : public Button {
public:
    explicit RandomizeButton(ProgressBar* bar) : Button("Randomize"), bar_(bar) {}
    void on_click() override {
        const f32 from = bar_->value();
        const f32 to = static_cast<f32>(std::rand() % 101) / 100.0f;
        AnimationSystem::instance().tween(from, to, 700.0f, Easing::OutCubic,
                                          [this](f32 v) { bar_->set_value(v); });
    }

private:
    ProgressBar* bar_;
};

// Animated overlay: backdrop fades in and panel slides up on open, reversed on close.
class DemoOverlay : public Overlay {
public:
    DemoOverlay() {
        title_ = new Label("Overlay with animation");
        title_->set_bold(true);
        body_ = new Label("Fade in + slide up on open,\nfade out + slide down on close.\nEsc or click outside to close.");
        body_->set_text_role(TextRole::Secondary);
        close_ = new CloseButton(this);
        close_->set_min_width(110.0f);
        close_->set_padding(16.0f);
        append_child(title_);
        append_child(body_);
        append_child(close_);
    }

    void show(Window& win) {
        const f32 pw = 380.0f, ph = 240.0f;
        const f32 left = (win.bounds().width() - pw) * 0.5f;
        const f32 top = (win.bounds().height() - ph) * 0.5f;
        set_panel_rect(RectF::make(left, top, pw, ph));
        title_->set_bounds(RectF::make(left + 24.0f, top + 20.0f, pw - 48.0f, 26.0f));
        body_->set_bounds(RectF::make(left + 24.0f, top + 56.0f, pw - 48.0f, 66.0f));
        close_->set_bounds(RectF::make(left + pw - 134.0f, top + ph - 54.0f, 110.0f, 34.0f));
        Overlay::show(win);
    }

private:
    class CloseButton : public Button {
    public:
        explicit CloseButton(DemoOverlay* owner) : Button("Close"), owner_(owner) {}
        void on_click() override { owner_->close(); }

    private:
        DemoOverlay* owner_;
    };

    Label* title_ = nullptr;
    Label* body_ = nullptr;
    CloseButton* close_ = nullptr;
};

class ShowOverlayButton : public Button {
public:
    ShowOverlayButton(Window& win, DemoOverlay* overlay)
        : Button("Show Overlay"), win_(win), overlay_(overlay) {}
    void on_click() override {
        if (overlay_->is_open()) {
            overlay_->close();
        } else {
            overlay_->show(win_);
        }
    }

private:
    Window& win_;
    DemoOverlay* overlay_;
};

class NotifyButton : public Button {
public:
    NotifyButton(Window& win, String text, NotificationType type)
        : Button(std::move(text)), win_(win), type_(type) {
        set_min_width(92.0f);
        set_padding(14.0f);
    }
    void on_click() override {
        NotificationManager::instance().show(
            win_, "Notification title", "This is a " + text() + " notification.\nClick it or wait to dismiss.",
            type_);
    }

private:
    Window& win_;
    NotificationType type_;
};

class DismissAllButton : public Button {
public:
    explicit DismissAllButton(Window& win) : Button("Dismiss All"), win_(win) {
        set_min_width(92.0f);
        set_padding(14.0f);
    }
    void on_click() override { NotificationManager::instance().dismiss_all(win_); }

private:
    Window& win_;
};

}  // namespace

namespace yzk {

// ===== Panel =====

Panel::Panel(Bg bg, f32 fixed_height) : bg_(bg), fixed_height_(fixed_height) {}

Size Panel::measure_content(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{0.0f, fixed_height_};
}

void Panel::arrange_content(const RectF& area, const PaintContext* ctx) {
    for (Widget* child = first_child(); child; child = child->next_sibling()) {
        if (!child->visible()) continue;
        child->set_bounds(area);
        child->perform_layout(ctx);
    }
}

void Panel::paint_impl(PaintContext& ctx) {
    const Theme& theme = Theme::get();
    const Color bg = bg_ == Bg::Window ? theme.background : theme.surface;
    const f32 radius = bg_ == Bg::Card ? theme.corner_radius : 0.0f;
    ctx.fill_rounded(bounds_, bg, radius);
    Widget::paint_impl(ctx);
}

// ===== EasingRow =====

EasingRow::EasingRow(String name, Easing easing)
    : name_(std::move(name)), easing_(easing) {
    set_cursor(Cursor::Hand);
}

Size EasingRow::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{0.0f, 36.0f};
}

void EasingRow::paint_impl(PaintContext& ctx) {
    ctx.draw_text_small(name_,
                        RectF::make(bounds_.left + 10.0f, bounds_.top, 84.0f, bounds_.height()),
                        demo::Text(), TextAlignH::Left, TextAlignV::Center);

    const RectF track = RectF::make(bounds_.left + 96.0f, bounds_.top + 8.0f,
                                    bounds_.width() - 106.0f, bounds_.height() - 16.0f);
    ctx.fill_rounded(track, has_flag(Flag_Hovered) ? demo::HoverBg() : demo::TrackBg(), 9.0f);

    const f32 pad = 9.0f;
    const f32 x = track.left + pad + (track.width() - pad * 2.0f) * value_;
    ctx.fill_circle(Point{x, track.top + track.height() * 0.5f}, 6.0f, demo::Accent());
}

void EasingRow::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseEnter:
            add_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseLeave:
            remove_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseUp:
            if (e.data.mouse.buttons == 0 || (e.data.mouse.buttons & MouseButton_Left)) {
                run();
                e.consumed = true;
            }
            break;

        default:
            break;
    }
}

void EasingRow::run() {
    value_ = 0.0f;
    invalidate();
    AnimationSystem::instance().tween(0.0f, 1.0f, 1200.0f, easing_,
                                      [this](f32 v) {
                                          value_ = v;
                                          invalidate();
                                      });
}

// ===== ColorCard =====

ColorCard::ColorCard() : color_(kPalette[0]) {
    set_cursor(Cursor::Hand);
}

Size ColorCard::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{0.0f, 88.0f};
}

void ColorCard::paint_impl(PaintContext& ctx) {
    ctx.fill_rounded(bounds_, color_, 10.0f);
    if (has_flag(Flag_Hovered)) ctx.draw_border(bounds_, demo::Text(), 1.0f, 10.0f);
    ctx.draw_text(kPaletteNames[index_ % kPaletteCount], bounds_, Color::rgba(0x14141CFF));
}

void ColorCard::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseEnter:
            add_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseLeave:
            remove_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseUp:
            if (e.data.mouse.buttons == 0 || (e.data.mouse.buttons & MouseButton_Left)) {
                const Color from = color_;
                index_ = (index_ + 1) % kPaletteCount;
                const Color to = kPalette[index_];
                AnimationSystem::instance().tween(0.0f, 1.0f, 600.0f, Easing::InOutQuad,
                                                  [this, from, to](f32 v) {
                                                      color_ = lerp_color(from, to, v);
                                                      invalidate();
                                                  });
                e.consumed = true;
            }
            break;

        default:
            break;
    }
}

// ===== ExpandingCard =====

ExpandingCard::ExpandingCard(String title, String body)
    : title_(std::move(title)), body_(std::move(body)) {
    set_cursor(Cursor::Hand);
    const char* p = body_.c_str();
    String line;
    while (*p) {
        if (*p == '\n') {
            lines_.push_back(line);
            line.clear();
        } else {
            line += *p;
        }
        ++p;
    }
    if (!line.empty()) lines_.push_back(line);
}

Size ExpandingCard::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    line_h_ = ctx->measure_text("Wg").height;
    f32 h = 40.0f;
    if (expanded_ || expand_progress_ > 0.0f) {
        h += (8.0f + (f32)lines_.size() * line_h_ + 12.0f) * expand_progress_;
    }
    return Size{0.0f, h};
}

void ExpandingCard::paint_impl(PaintContext& ctx) {
    const f32 radius = 10.0f;
    ctx.fill_rounded(bounds_, demo::CardBg(), radius);
    ctx.draw_border(bounds_, has_flag(Flag_Hovered) ? demo::CardBorder() : demo::TrackBg(), 1.0f, radius);

    ctx.draw_text_small(title_,
                        RectF::make(bounds_.left + 16.0f, bounds_.top, bounds_.width() - 80.0f, 40.0f),
                        demo::Text(), TextAlignH::Left, TextAlignV::Center);
    ctx.draw_text_small(expanded_ ? "Hide" : "Show",
                        RectF::make(bounds_.right - 64.0f, bounds_.top, 34.0f, 40.0f),
                        demo::TextSecondary(), TextAlignH::Left, TextAlignV::Center);

    const FontId icon_font = ctx.font(icon_family, 12.0f);
    ctx.draw_text(icon_font,
                  icon_glyph(expanded_ ? IconId::CaretDown : IconId::CaretRight),
                  RectF::make(bounds_.right - 30.0f, bounds_.top, 18.0f, 40.0f),
                  demo::TextSecondary());

    if (expand_progress_ > 0.0f) {
        const f32 body_h = bounds_.height() - 40.0f;
        if (body_h > 4.0f) {
            const RectF body = RectF::make(bounds_.left + 12.0f, bounds_.top + 40.0f,
                                           bounds_.width() - 24.0f, body_h);
            ctx.fill_rounded(body, demo::TrackBg(), 6.0f);
            const f32 content_h = body.height() - 12.0f;
            const size_t visible = static_cast<size_t>(content_h / line_h_);
            for (size_t i = 0; i < visible && i < lines_.size(); ++i) {
                ctx.draw_text_small(lines_[i],
                                    RectF::make(body.left + 10.0f,
                                                body.top + 6.0f + (f32)i * line_h_,
                                                body.width() - 20.0f, line_h_),
                                    demo::Text());
            }
        }
    }
}

void ExpandingCard::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseEnter:
            add_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseLeave:
            remove_flag(Flag_Hovered);
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseUp:
            if (e.data.mouse.buttons == 0 || (e.data.mouse.buttons & MouseButton_Left)) {
                expanded_ = !expanded_;
                AnimationSystem& as = AnimationSystem::instance();
                as.tween(expand_progress_, expanded_ ? 1.0f : 0.0f, 180.0f,
                         expanded_ ? Easing::OutCubic : Easing::InCubic,
                         [this](f32 v) {
                             expand_progress_ = v;
                             invalidate();
                         });
                invalidate();
                e.consumed = true;
            }
            break;

        default:
            break;
    }
}

// ===== Visual transform demo (opacity / translate / rotate / scale) =====
// Only visual state changes (set_scale/set_rotate_deg/set_opacity/set_translate);
// hit testing still uses the layout rect, like CSS transform.
enum class TransformMode { ScaleHover, SpinClick, FadeClick };

class TransformCard : public Widget {
public:
    TransformCard(String label, Color color, TransformMode mode)
        : label_(std::move(label)), color_(color), mode_(mode) {
        set_cursor(Cursor::Hand);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override {
        (void)available;
        (void)ctx;
        return Size{96.0f, 96.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        const RectF b = bounds_;
        ctx.fill_rounded(b, color_, 12.0f);
        ctx.draw_text_small(label_, b, demo::Text(), TextAlignH::Center, TextAlignV::Center);
    }

    void on_event(Event& e) override {
        AnimationSystem& as = AnimationSystem::instance();
        switch (e.type) {
            case EventType::MouseEnter:
                add_flag(Flag_Hovered);
                if (mode_ == TransformMode::ScaleHover) {
                    as.tween(scale_x(), 1.08f, 160.0f, Easing::OutCubic,
                             [this](f32 v) { set_scale(v, v); });
                }
                e.consumed = true;
                break;

            case EventType::MouseLeave:
                remove_flag(Flag_Hovered);
                if (mode_ == TransformMode::ScaleHover) {
                    as.tween(scale_x(), 1.0f, 200.0f, Easing::OutCubic,
                             [this](f32 v) { set_scale(v, v); });
                }
                e.consumed = true;
                break;

            case EventType::MouseDown:
                if ((e.data.mouse.buttons & MouseButton_Left) != 0) {
                    if (mode_ == TransformMode::SpinClick) {
                        as.tween(rotate_deg(), rotate_deg() + 360.0f, 700.0f, Easing::OutBack,
                                 [this](f32 v) { set_rotate_deg(v); });
                    } else if (mode_ == TransformMode::FadeClick) {
                        if (faded_) {
                            as.tween(0.15f, 1.0f, 260.0f, Easing::OutCubic,
                                     [this](f32 v) {
                                         set_opacity(v);
                                         set_translate(0.0f, (1.0f - v) * 6.0f);
                                     });
                        } else {
                            as.tween(1.0f, 0.15f, 260.0f, Easing::InCubic,
                                     [this](f32 v) {
                                         set_opacity(v);
                                         set_translate(0.0f, (1.0f - v) * 6.0f);
                                     });
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
    TransformMode mode_;
    bool faded_ = false;
};

// ===== Page =====

Widget* make_animation_page(Window& win) {
    auto root = new DockPanel;

    // Title bar: title + Run All button
    auto topbar = new Panel(Panel::Bg::Window, 48.0f);
    root->dock(topbar, Dock::Top);
    auto top_row = new DockPanel;
    top_row->set_padding(14.0f);
    topbar->append_child(top_row);

    auto title = new Label("Animation Gallery");
    title->set_bold(true);
    title->set_align(TextAlignH::Left, TextAlignV::Center);
    top_row->dock(title, Dock::Left);

    // Left: easing gallery
    auto gallery = new Panel(Panel::Bg::Card);
    gallery->set_min_size(Size{330.0f, 0.0f});
    root->dock(gallery, Dock::Left);

    auto gallery_scroll = new ScrollView;
    gallery->append_child(gallery_scroll);
    auto easing_stack = new StackPanel(Orientation::Vertical);
    easing_stack->set_spacing(6.0f);
    gallery_scroll->set_content(easing_stack);

    struct EasingEntry {
        const char* name;
        Easing easing;
    };
    static const EasingEntry kEasings[] = {
        {"Linear", Easing::Linear},
        {"InQuad", Easing::InQuad},
        {"OutQuad", Easing::OutQuad},
        {"InOutQuad", Easing::InOutQuad},
        {"InCubic", Easing::InCubic},
        {"OutCubic", Easing::OutCubic},
        {"InOutCubic", Easing::InOutCubic},
        {"InBack", Easing::InBack},
        {"OutBack", Easing::OutBack},
        {"InOutBack", Easing::InOutBack},
    };
    std::vector<EasingRow*> rows;
    for (const EasingEntry& entry : kEasings) {
        auto row = new EasingRow(entry.name, entry.easing);
        easing_stack->append_child(row);
        rows.push_back(row);
    }
    auto gallery_hint = new Label("Click a row to run");
    gallery_hint->set_text_role(TextRole::Secondary);
    gallery_hint->set_small(true);
    gallery_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    easing_stack->append_child(gallery_hint);

    auto run_all = new RunAllButton(rows);
    run_all->set_min_width(90.0f);
    run_all->set_padding(14.0f);
    top_row->dock(run_all, Dock::Right);
    TooltipManager::instance().set_tooltip(run_all, "Run all 10 easing rows at once");

    // Right: color transition, expand card, progress bar
    auto right_stack = new StackPanel(Orientation::Vertical);
    right_stack->set_padding(18.0f);
    right_stack->set_spacing(14.0f);
    root->dock(right_stack, Dock::Fill);

    auto color_hint = new Label("Color transition - click the card");
    color_hint->set_text_role(TextRole::Secondary);
    color_hint->set_small(true);
    color_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    right_stack->append_child(color_hint);

    auto color_card = new ColorCard;
    right_stack->append_child(color_card);

    auto expand_hint = new Label("Expand / collapse - click the card");
    expand_hint->set_text_role(TextRole::Secondary);
    expand_hint->set_small(true);
    expand_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    right_stack->append_child(expand_hint);

    auto expand_card = new ExpandingCard(
        "Animation system",
        "Tween: value interpolation over time\nEasing: 10 built-in curves\nDriver: automatic 16ms timer while animating");
    right_stack->append_child(expand_card);

    auto progress_hint = new Label("Value animation - click Randomize");
    progress_hint->set_text_role(TextRole::Secondary);
    progress_hint->set_small(true);
    progress_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    right_stack->append_child(progress_hint);

    auto progress_row = new DockPanel;
    progress_row->set_min_size(Size{0.0f, 32.0f});
    right_stack->append_child(progress_row);

    auto progress_label = new Label("Progress");
    progress_label->set_align(TextAlignH::Left, TextAlignV::Center);
    progress_label->set_min_width(80.0f);
    progress_row->dock(progress_label, Dock::Left);

    auto bar = new ProgressBar;
    progress_row->dock(bar, Dock::Fill);

    auto randomize = new RandomizeButton(bar);
    randomize->set_min_width(100.0f);
    randomize->set_padding(14.0f);
    progress_row->dock(randomize, Dock::Right);

    auto list_hint = new Label("ListView - 100,000 items, virtualized + scrollbar");
    list_hint->set_text_role(TextRole::Secondary);
    list_hint->set_small(true);
    list_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    right_stack->append_child(list_hint);

    auto list = new ListView;
    list->set_row_height(26.0f);
    list->set_max_size(Size{1e7f, 170.0f});
    {
        std::vector<String> items;
        items.reserve(100000);
        for (i32 i = 0; i < 100000; ++i) items.push_back("Item #" + std::to_string(i));
        list->set_items(std::move(items));
    }
    list->set_selected(5);
    right_stack->append_child(list);
    TooltipManager::instance().set_tooltip(randomize, "Randomize the progress value");

    auto overlay_hint = new Label("Overlay - click to show");
    overlay_hint->set_text_role(TextRole::Secondary);
    overlay_hint->set_small(true);
    overlay_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    right_stack->append_child(overlay_hint);

    auto overlay = new DemoOverlay;
    auto overlay_btn = new ShowOverlayButton(win, overlay);
    overlay_btn->set_min_width(140.0f);
    overlay_btn->set_padding(16.0f);
    right_stack->append_child(overlay_btn);
    TooltipManager::instance().set_tooltip(overlay_btn, "Fade + slide overlay with dim backdrop");

    auto notif_hint = new Label("Notifications - bottom right, stacked");
    notif_hint->set_text_role(TextRole::Secondary);
    notif_hint->set_small(true);
    notif_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    right_stack->append_child(notif_hint);

    auto notif_row = new DockPanel;
    notif_row->set_min_size(Size{0.0f, 32.0f});
    right_stack->append_child(notif_row);

    auto notif_info = new NotifyButton(win, "Info", NotificationType::Info);
    auto notif_success = new NotifyButton(win, "Success", NotificationType::Success);
    auto notif_warning = new NotifyButton(win, "Warning", NotificationType::Warning);
    auto notif_error = new NotifyButton(win, "Error", NotificationType::Error);
    auto notif_clear = new DismissAllButton(win);
    notif_row->dock(notif_info, Dock::Left);
    notif_row->dock(notif_success, Dock::Left);
    notif_row->dock(notif_warning, Dock::Left);
    notif_row->dock(notif_error, Dock::Left);
    notif_row->dock(notif_clear, Dock::Left);

    auto image_hint = new Label("Image - PNG via WIC, rounded corners");
    image_hint->set_text_role(TextRole::Secondary);
    image_hint->set_small(true);
    image_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    right_stack->append_child(image_hint);

    auto image_row = new StackPanel(Orientation::Horizontal);
    image_row->set_spacing(10.0f);
    right_stack->append_child(image_row);

    auto make_thumb = [&](f32 radius) {
        auto img = new Image;
        img->set_corner_radius(radius);
        img->set_max_height(96.0f);
        img->load_from_file(win, "testimg.jpg");
        return img;
    };

    image_row->append_child(make_thumb(4.0f));   // r4: small thumbnails, avatars
    image_row->append_child(make_thumb(8.0f));   // r8: small card images in lists, most common
    image_row->append_child(make_thumb(12.0f));  // r12: regular content card images (feed covers)

    auto image_spec = new Label("r4 thumbnail/avatar · r8 list card · r12 content card");
    image_spec->set_text_role(TextRole::Secondary);
    image_spec->set_small(true);
    image_spec->set_align(TextAlignH::Left, TextAlignV::Center);
    right_stack->append_child(image_spec);

    auto menu_hint = new Label("ContextMenu - right-click the card below");
    menu_hint->set_text_role(TextRole::Secondary);
    menu_hint->set_small(true);
    menu_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    right_stack->append_child(menu_hint);

    auto menu_host = new MenuHost(win);
    right_stack->append_child(menu_host);

    auto theme_hint = new Label("Theme - switch between dark and light");
    theme_hint->set_text_role(TextRole::Secondary);
    theme_hint->set_small(true);
    theme_hint->set_align(TextAlignH::Left, TextAlignV::Center);
    right_stack->append_child(theme_hint);

    auto theme_btn = new ToggleThemeButton(win);
    theme_btn->set_min_width(140.0f);
    theme_btn->set_padding(16.0f);
    right_stack->append_child(theme_btn);
    TooltipManager::instance().set_tooltip(theme_btn, "Switch Material dark / light theme");

    return root;
}

}  // namespace yzk
