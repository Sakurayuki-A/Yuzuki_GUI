#include "app.hpp"

#include <yuzuki/yuzuki.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace yzk {

// =====================================================================
// Playground: API dogfooding app. EVERYTHING below uses only the public
// API surface (<yuzuki/yuzuki.hpp>).
// =====================================================================

enum class PageId { Widgets, Todo, Custom, Feedback, Theme, Layouts };

// ---------------------------------------------------------------------
// Sidebar
// ---------------------------------------------------------------------

struct NavState {
    std::vector<std::pair<PageId, Widget*>> pages;
};

struct NavItem : Widget {
    String label;
    bool active = false;
    PageId page_id;

    NavItem(String text, PageId id)
        : label(std::move(text)), page_id(id) {
        set_cursor(Cursor::Hand);
    }

    Size measure_impl(Size, const PaintContext*) override {
        return Size{0.0f, 34.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        const Theme& th = Theme::get();
        if (active) {
            ctx.fill_rounded(bounds_, th.accent, 6.0f);
        } else if (hovered()) {  // hover flag maintained centrally by the Window
            ctx.fill_rounded(bounds_, th.surface_container_high, 6.0f);
        }
        ctx.draw_text(label, bounds_, active ? th.accent_text : th.text,
                      TextAlignH::Center, TextAlignV::Center);
    }

    void on_event(Event& e) override {
        switch (e.type) {
            case EventType::MouseDown:
                if (e.data.mouse.buttons & MouseButton_Left) {
                    add_flag(Flag_Pressed);
                    invalidate();
                    e.consumed = true;
                }
                break;
            case EventType::MouseUp:
                // The Window now re-injects the just-released button into the
                // mouse.buttons mask, so activation can test the button directly
                // (no longer cleared on WM_*BUTTONUP).
                if (has_flag(Flag_Pressed)) {
                    remove_flag(Flag_Pressed);
                    invalidate();
                    if (e.data.mouse.buttons & MouseButton_Left) on_activate();
                    e.consumed = true;
                }
                break;
            default:
                break;
        }
    }

    std::function<void()> on_activate_cb_;
    NavItem& set_on_activate(std::function<void()> cb) {
        on_activate_cb_ = std::move(cb);
        return *this;
    }
    virtual void on_activate() {
        if (on_activate_cb_) on_activate_cb_();
    }
};

struct AppNav {
    struct Entry {
        Widget* nav = nullptr;
        Widget* page = nullptr;
        PageId id;
    };
    std::vector<Entry> entries;
    Widget* pages_host = nullptr;

    void activate(PageId id) {
        for (auto& e : entries) {
            const bool on = (e.id == id);
            auto* nav = static_cast<NavItem*>(e.nav);
            nav->active = on;
            e.page->set_visible(on);
            nav->invalidate();
        }
        if (pages_host) pages_host->invalidate();
    }
};

// ---------------------------------------------------------------------
// Widgets page (all built-in controls, public API only)
// ---------------------------------------------------------------------

Widget* build_widgets_page() {
    auto* root = new DockPanel;
    auto* scroll = new ScrollView;
    root->dock(scroll, Dock::Fill);

    auto* body = new FlexBox(Orientation::Vertical);
    body->set_padding(20.0f);
    body->set_spacing(6.0f);
    body->set_align_cross(FlexCrossAlign::Stretch);
    scroll->set_content(body);

    auto heading = [body](const char* t) {
        auto* l = new Label(t);
        l->set_bold(true).set_align(TextAlignH::Left, TextAlignV::Center)
            .set_margin(Margins{0, 10, 0, 2});
        body->append_child(l);
        return l;
    };

    auto row = [body](f32 spacing) {
        auto* r = new FlexBox;
        r->set_spacing(spacing).set_align_cross(FlexCrossAlign::Center);
        body->append_child(r);
        return r;
    };

    // Labels
    heading("Labels");
    {
        auto* r = row(12.0f);
        r->add<Label>("Default");
        r->add<Label>("Small").small(true);
        r->add<Label>("Bold").bold(true);
        r->add<Label>("Secondary").text_role(TextRole::Secondary);
        r->add<Label>("Disabled").text_role(TextRole::Disabled);
    }

    // Buttons
    heading("Buttons");
    {
        auto* r = row(10.0f);
        r->add<Button>("Primary");
        r->add<Button>("Secondary").set_accent(false);
        r->add<Button>("Wide").set_min_width(140.0f);
        // Ideal Lego form: no new, no parens, child is auto-attached, chain flows.
        r->add<Button>("Chained")
            .on_click([]() {})
            .padding(14)
            .width(110);
    }

    // TextBoxes
    heading("TextBox");
    {
        auto* r = row(10.0f);
        r->add<TextBox>(String(), TextBoxConfig{});
        TextBoxConfig pwd;
        pwd.mode = TextBoxMode::Password;
        r->add<TextBox>(String(), pwd);
        r->add<TextBox>(String(), TextBoxConfig{}).set_placeholder("Type here...");
    }

    // Selection controls
    heading("Selection Controls");
    {
        auto* r = row(16.0f);
        r->add<CheckBox>("Check me").checked(true);
        r->add<ToggleSwitch>().checked(true);
        r->add<RadioButton>("A").checked(true);
        r->add<RadioButton>("B");
    }

    // Slider + Progress
    heading("Slider & ProgressBar");
    {
        auto* r = row(10.0f);
        r->add<Slider>().set_range(0.0f, 100.0f).set_value(60.0f);
        r->add<ProgressBar>().set_value(0.6f);
    }

    // SpinBox + ComboBox
    heading("SpinBox & ComboBox");
    {
        auto* r = row(10.0f);
        r->add<SpinBox>(42.0, 0.0, 100.0, 1.0);
        // braced-init-list can't be deduced by the template Args..., so pass a
        // std::vector explicitly.
        r->add<ComboBox>(std::vector<yzk::String>{"Apple", "Banana", "Cherry", "Date"})
            .set_selected_index(0);
    }

    // Box
    heading("Box");
    {
        auto* r = row(10.0f);
        auto* card = new Box;
        card->bg_role(ThemeRole::SurfaceContainer)  // theme-resolved at paint time
            .padding(16.0f)
            .radius(Theme::get().corner_radius)
            .shadow(8.0f, 4.0f);
        card->add<Label>("Shadow card");
        r->append_child(card);

        auto* bordered = new Box;
        bordered->bg_role(ThemeRole::Surface)
            .padding(16.0f)
            .radius(Theme::get().corner_radius)
            .set_border(2.0f, Theme::get().border);
        bordered->add<Label>("Bordered");
        r->append_child(bordered);
    }

    // StackPanel main-axis fill: children with flex_grow()>0 split free space
    heading("StackPanel fill");
    {
        auto* sp = new StackPanel(Orientation::Horizontal);
        sp->set_spacing(8.0f).set_fill(true).set_min_size(Size{0.0f, 48.0f});
        auto* a = new Box;
        a->bg_role(ThemeRole::Accent).radius(4.0f).set_min_size(Size{80.0f, 0.0f});
        sp->append_child(a);
        auto* b = new Box;
        b->bg_role(ThemeRole::SurfaceContainerHigh).radius(4.0f)
            .set_min_width(40.0f).set_flex_grow(1.0f);  // takes the leftover main-axis space
        sp->append_child(b);
        body->append_child(sp);
    }

    // Layout demos
    heading("FlexBox");
    {
        auto* r = row(8.0f);
        for (int i = 0; i < 5; ++i) {
            r->add<Box>().bg_role(ThemeRole::Accent)
                .radius(4.0f).set_min_size(Size{32.0f, 32.0f});
        }
    }

    heading("WrapPanel");
    {
        auto* wrap = new WrapPanel;
        wrap->set_spacing(6.0f);
        const char* tags[] = {"React", "Vue", "Svelte", "Angular", "Solid", "Qwik", "Preact"};
        for (const char* t : tags) wrap->add<Button>(t).set_accent(false);
        body->append_child(wrap);
    }

    heading("GridPanel");
    {
        auto* grid = new GridPanel(2, 2);
        grid->set_gap(8.0f);
        ThemeRole roles[] = {ThemeRole::Accent, ThemeRole::SurfaceContainerHigh,
                             ThemeRole::SurfaceContainer, ThemeRole::Border};
        Box* cells[4];
        for (int i = 0; i < 4; ++i) {
            cells[i] = new Box;
            cells[i]->bg_role(roles[i]).radius(4.0f).set_min_size(Size{56.0f, 40.0f});
        }
        grid->add(cells[0], 0, 0);
        grid->add(cells[1], 1, 0);
        grid->add(cells[2], 0, 1);
        grid->add(cells[3], 1, 1);
        body->append_child(grid);
    }

    return root;
}

// ---------------------------------------------------------------------
// Todo page
// ---------------------------------------------------------------------

struct TodoItem {
    String text;
    bool done = false;
};

struct TodoSource : ListView::DataSource {
    std::vector<TodoItem> items;
    i32 filter = 0;

    i32 count() const override {
        if (filter == 0) return static_cast<i32>(items.size());
        i32 n = 0;
        for (auto& it : items) {
            if (filter == 1 && !it.done) ++n;
            if (filter == 2 && it.done) ++n;
        }
        return n;
    }

    String text_at(i32 idx) const override {
        const i32 ri = real_index(idx);
        return ri < 0 ? String{} : items[ri].text;
    }

    i32 real_index(i32 idx) const {
        i32 n = 0;
        for (i32 i = 0; i < static_cast<i32>(items.size()); ++i) {
            if (filter == 1 && items[i].done) continue;
            if (filter == 2 && !items[i].done) continue;
            if (n == idx) return i;
            ++n;
        }
        return -1;
    }
};

struct TodoRowDelegate : ListView::RowDelegate {
    TodoSource& src;
    explicit TodoRowDelegate(TodoSource& s) : src(s) {}

    void draw(ListView&, PaintContext& ctx, i32 index,
              const RectF& r) override {
        const Theme& th = Theme::get();
        const i32 ri = src.real_index(index);
        if (ri < 0) return;
        auto& item = src.items[ri];

        const RectF cb = {r.left + 8.0f, r.top + 6.0f, r.left + 22.0f, r.top + 20.0f};
        ctx.fill_rounded(cb, item.done ? th.accent : Color{}, 3.0f);
        if (item.done) {
            // checkmark glyph: draw a white dot instead of relying on font coverage
            ctx.fill_circle(Point{(cb.left + cb.right) / 2.0f, (cb.top + cb.bottom) / 2.0f},
                            2.5f, th.accent_text);
        }
        const RectF tr = {r.left + 30.0f, r.top, r.right - 8.0f, r.bottom};
        ctx.draw_text(item.text, tr, item.done ? th.text_disabled : th.text,
                      TextAlignH::Left, TextAlignV::Center);
    }
};

struct TodoPage {
    TodoSource source;
    TodoRowDelegate delegate{source};
    ListView* list = nullptr;
    TextBox* input = nullptr;
    Label* count = nullptr;
    ComboBox* filter = nullptr;

    void refresh() {
        if (list) list->invalidate();
        update_count();
    }

    void update_count() {
        if (!count) return;
        i32 done = 0;
        for (auto& it : source.items) if (it.done) ++done;
        char buf[64];
        snprintf(buf, sizeof(buf), "%d/%d done", done,
                 static_cast<i32>(source.items.size()));
        count->set_text(buf);
    }

    void add(const String& text) {
        if (text.empty()) return;
        source.items.push_back({text, false});
        refresh();
    }

    void toggle(i32 idx) {
        const i32 ri = source.real_index(idx);
        if (ri >= 0) {
            source.items[ri].done = !source.items[ri].done;
            refresh();
        }
    }

    void remove_selected() {
        if (!list || list->selected() < 0) return;
        const i32 ri = source.real_index(list->selected());
        if (ri >= 0) {
            source.items.erase(source.items.begin() + ri);
            // Reset the selection so the now-stale index isn't used again.
            list->set_selected(-1);
            refresh();
        }
    }
};

Widget* build_todo_page(Window& win) {
    auto* page = new TodoPage;
    (void)win;

    auto* root = new DockPanel;

    // Top bar
    auto* top = new StackPanel(Orientation::Horizontal);
    top->set_spacing(8.0f).set_padding(16.0f);  // derived setter first, base last
    root->dock(top, Dock::Top);

    auto* title = new Label("Todo");
    title->bold(true).set_align(TextAlignH::Left, TextAlignV::Center);
    top->append_child(title);

    // One expression builds a control and wires its behaviour.
    page->input = new TextBox(String(), TextBoxConfig{});
    page->input->set_placeholder("Add a task...").set_min_width(220.0f);
    top->append_child(page->input);

    top->add<Button>("Add").on_click([page]() {
        page->add(page->input->text());
        page->input->set_text({});
    });

    // Delete the currently highlighted row.
    top->add<Button>("Delete").set_accent(false).on_click([page]() {
        page->remove_selected();
    });

    page->filter = new ComboBox({"All", "Active", "Done"});
    page->filter->on_changed([page](i32 index) {
        page->source.filter = index;
        page->refresh();
    });
    top->append_child(page->filter);

    page->count = new Label("0/0 done");
    page->count->text_role(TextRole::Secondary)
        .set_align(TextAlignH::Right, TextAlignV::Center);
    top->append_child(page->count);

    // List
    auto* lv = new ListView;
    lv->on_selected([page](i32 index) { page->toggle(index); });
    lv->set_data_source(&page->source).set_row_delegate(&page->delegate).set_row_height(28.0f);
    page->list = lv;
    root->dock(lv, Dock::Fill);

    // Seed items (also exercises DataSource + RowDelegate)
    page->add("Learn the public API");
    page->add("Build with only public headers");
    page->add("Record every API gap");
    page->source.items[0].done = true;
    page->refresh();

    return root;
}

// ---------------------------------------------------------------------
// Custom widgets page (subclass Widget: paint/measure/event overrides)
// ---------------------------------------------------------------------

struct Gauge : Widget {
    f32 value = 0.0f;
    f32 min_v = 0.0f;
    f32 max_v = 100.0f;
    bool dragging = false;

    AnimatableProperty<f32> anim{0.0f};

    Gauge() {
        anim.set_transition(160.0f);
        anim.set_on_changed([this](const f32&) { invalidate(); });
    }

    Size measure_impl(Size available, const PaintContext*) override {
        const f32 s = std::min(available.width, 150.0f);
        return Size{s, s};
    }

    void paint_impl(PaintContext& ctx) override {
        const Theme& th = Theme::get();
        const RectF b = bounds_;
        const f32 cx = b.left + b.width() / 2.0f;
        const f32 cy = b.top + b.height() / 2.0f;
        const f32 outer = std::min(b.width(), b.height()) / 2.0f - 4.0f;
        const f32 inner = outer - 12.0f;

        ctx.fill_sweep_gradient(b, Point{cx, cy}, -135.0f, 270.0f,
                                th.surface_container_high, th.surface_container_high, outer);

        const f32 frac = std::clamp(anim.value() / max_v, 0.0f, 1.0f);
        if (frac > 0.001f) {
            ctx.fill_sweep_gradient_stops(
                b, Point{cx, cy}, -135.0f, frac * 270.0f,
                std::vector<GradientStop>{{0.0f, th.accent},
                                          {1.0f, th.accent_hover}},
                outer);
        }

        ctx.fill_circle(Point{cx, cy}, inner, th.background);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f", anim.value());
        ctx.draw_text(buf, RectF::make(cx - 28.0f, cy - 12.0f, 56.0f, 24.0f),
                      th.text, TextAlignH::Center, TextAlignV::Center);
        ctx.draw_text_small("VALUE", RectF::make(cx - 24.0f, cy + 10.0f, 48.0f, 14.0f),
                            th.text_secondary, TextAlignH::Center, TextAlignV::Top);
    }

    void on_event(Event& e) override {
        switch (e.type) {
            case EventType::MouseDown:
                if (e.data.mouse.buttons & MouseButton_Left) {
                    dragging = true;
                    apply_mouse(e.data.mouse.x, e.data.mouse.y);
                    e.consumed = true;
                }
                break;
            case EventType::MouseMove:
                if (dragging) {
                    apply_mouse(e.data.mouse.x, e.data.mouse.y);
                    e.consumed = true;
                }
                break;
            case EventType::MouseUp:
                if (dragging) {
                    dragging = false;
                    e.consumed = true;
                }
                break;
            default:
                break;
        }
    }

    void apply_mouse(f32 mx, f32 my) {
        const RectF b = bounds_;
        const f32 dx = mx - (b.left + b.width() / 2.0f);
        const f32 dy = my - (b.top + b.height() / 2.0f);
        f32 angle = std::atan2(dx, -dy) * 180.0f / 3.14159265358979323846f;
        f32 frac = (angle + 135.0f) / 270.0f;
        frac = std::clamp(frac, 0.0f, 1.0f);
        const f32 v = min_v + frac * (max_v - min_v);
        if (anim.value() != v) anim = v;
    }
};

struct Card : Widget {
    String title;
    String subtitle;
    Color accent;

    Card(String t, String sub, Color c)
        : title(std::move(t)), subtitle(std::move(sub)), accent(c) {}

    Size measure_impl(Size, const PaintContext*) override {
        return Size{190.0f, 104.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        const Theme& th = Theme::get();
        const RectF b = bounds_;
        ctx.draw_shadow(b, th.corner_radius, 10.0f, Color{0, 0, 0, 40});
        ctx.fill_rounded(b, th.surface_container, th.corner_radius);
        ctx.fill_rounded(RectF::make(b.left, b.top, b.width(), 4.0f), accent, 4.0f);
        ctx.draw_text(title, RectF::make(b.left + 14.0f, b.top + 18.0f,
                                         b.width() - 28.0f, 22.0f),
                      th.text, TextAlignH::Left, TextAlignV::Center);
        ctx.draw_text_small(subtitle, RectF::make(b.left + 14.0f, b.top + 46.0f,
                                                  b.width() - 28.0f, 18.0f),
                            th.text_secondary, TextAlignH::Left, TextAlignV::Top);
        ctx.draw_border(b, th.border, 1.0f, th.corner_radius);
    }
};

struct AnimatedToggle : Widget {
    bool checked = false;
    AnimatableProperty<f32> knob{0.0f};

    AnimatedToggle() {
        knob.set_transition(180.0f);
        knob.set_on_changed([this](const f32&) { invalidate(); });
        set_cursor(Cursor::Hand);
    }

    Size measure_impl(Size, const PaintContext*) override {
        return Size{44.0f, 24.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        const Theme& th = Theme::get();
        const RectF b = bounds_;
        ctx.fill_rounded(b, checked ? th.accent : th.surface_container_high,
                         b.height() / 2.0f);
        const f32 r = b.height() / 2.0f - 2.0f;
        const f32 cx = b.left + (checked ? b.width() - r - 2.0f : r + 2.0f);
        const f32 cy = b.top + b.height() / 2.0f;
        // knob.value() drives a small slide on hover; position snaps via `checked`
        ctx.fill_circle(Point{cx + knob.value(), cy}, r, Color{255, 255, 255});
    }

    void on_event(Event& e) override {
        if (e.type == EventType::MouseUp) {
            checked = !checked;
            knob = 0.0f;
            invalidate();
            e.consumed = true;
        }
    }
};

Widget* build_custom_page(Window& win) {
    auto* root = new DockPanel;
    auto* scroll = new ScrollView;
    root->dock(scroll, Dock::Fill);

    auto* body = new FlexBox(Orientation::Vertical);
    body->set_padding(20.0f);
    body->set_spacing(16.0f);
    body->set_align_cross(FlexCrossAlign::Start);
    scroll->set_content(body);

    body->add<Label>("Gauge — drag to adjust")
        .set_bold(true)
        .set_align(TextAlignH::Left, TextAlignV::Center);

    auto* g1 = new Gauge;
    g1->max_v = 100.0f;
    g1->anim.set(72.0f);
    body->append_child(g1);

    body->add<Label>("Cards")
        .set_bold(true)
        .set_align(TextAlignH::Left, TextAlignV::Center);

    auto* row = new FlexBox;
    row->set_spacing(16.0f);
    const Theme& th = Theme::get();
    row->add<Card>("Performance", "60 FPS, 2.1 ms/frame", th.accent);
    row->add<Card>("Memory", "128 MB used", Color{0x4C, 0xAF, 0x50});
    row->add<Card>("Uptime", "99.97%", Color{0xFF, 0x98, 0x00});
    body->append_child(row);

    body->add<Label>("AnimatedToggle")
        .set_bold(true)
        .set_align(TextAlignH::Left, TextAlignV::Center);

    auto* tog_row = new FlexBox;
    tog_row->set_spacing(12.0f);
    for (int i = 0; i < 3; ++i) {
        auto& t = tog_row->add<AnimatedToggle>();
        if (i == 0) t.checked = true;
    }
    body->append_child(tog_row);

    return root;
}

// ---------------------------------------------------------------------
// Feedback page (Notification / ContextMenu / Tooltip / Overlay)
// ---------------------------------------------------------------------

struct NotifyBtn : Button {
    Window& win;
    NotificationType type;
    NotifyBtn(Window& w, const char* text, NotificationType t)
        : Button(text), win(w), type(t) {
        set_cursor(Cursor::Hand);
    }
    void on_click() override {
        NotificationManager::instance().show(
            win, text(), "This is a " + text() + " notification.\nClick or wait to dismiss.", type);
    }
};

class MenuHost : public Widget {
public:
    explicit MenuHost(Window& win) : win_(win) {
        menu_.add_item("Copy text", [this] { notify("Copied"); });
        menu_.add_item("Run action", [this] { notify("Action ran"); });
        menu_.add_separator();
        menu_.add_item("About", [this] { notify("ContextMenu demo"); });
    }

    Size measure_impl(Size, const PaintContext*) override {
        return Size{0.0f, 44.0f};
    }

    void paint_impl(PaintContext& ctx) override {
        const Theme& th = Theme::get();
        ctx.fill_rounded(bounds_, th.surface, 8.0f);
        ctx.draw_border(bounds_, th.border, 1.0f, 8.0f);
        ctx.draw_text("Right-click me", bounds_, th.text);
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

class DemoOverlay : public Overlay {
public:
    DemoOverlay() {
        title_ = new Label("Overlay");
        title_->bold(true);
        body_ = new Label("Fade + slide. Esc or click outside to close.");
        body_->text_role(TextRole::Secondary);
        close_ = new CloseBtn(this);
        close_->set_min_width(100.0f).padding(14.0f);
        append_child(title_);
        append_child(body_);
        append_child(close_);
    }

    void show(Window& win) {
        const f32 pw = 340.0f, ph = 180.0f;
        const f32 left = (win.bounds().width() - pw) * 0.5f;
        const f32 top = (win.bounds().height() - ph) * 0.5f;
        set_panel_rect(RectF::make(left, top, pw, ph));
        title_->set_bounds(RectF::make(left + 24.0f, top + 20.0f, pw - 48.0f, 26.0f));
        body_->set_bounds(RectF::make(left + 24.0f, top + 56.0f, pw - 48.0f, 40.0f));
        close_->set_bounds(RectF::make(left + pw - 124.0f, top + ph - 50.0f, 100.0f, 34.0f));
        Overlay::show(win);
    }

private:
    struct CloseBtn : Button {
        DemoOverlay* owner;
        CloseBtn(DemoOverlay* o) : Button("Close"), owner(o) {}
        void on_click() override { owner->close(); }
    };
    Label* title_ = nullptr;
    Label* body_ = nullptr;
    CloseBtn* close_ = nullptr;
};

Widget* build_feedback_page(Window& win) {
    auto* root = new DockPanel;
    auto* scroll = new ScrollView;
    root->dock(scroll, Dock::Fill);

    auto* body = new FlexBox(Orientation::Vertical);
    body->set_padding(20.0f);
    body->set_spacing(6.0f);
    body->set_align_cross(FlexCrossAlign::Stretch);
    scroll->set_content(body);

    auto heading = [body](const char* t) {
        auto* l = new Label(t);
        l->set_bold(true).set_align(TextAlignH::Left, TextAlignV::Center)
            .set_margin(Margins{0, 10, 0, 2});
        body->append_child(l);
        return l;
    };

    auto row = [body](f32 spacing) {
        auto* r = new FlexBox;
        r->set_spacing(spacing).set_align_cross(FlexCrossAlign::Center);
        body->append_child(r);
        return r;
    };

    auto hint = [body](const char* t) {
        auto* l = new Label(t);
        l->text_role(TextRole::Secondary).small(true)
            .set_align(TextAlignH::Left, TextAlignV::Center);
        body->append_child(l);
        return l;
    };

    heading("Notifications");
    {
        auto* r = row(8.0f);
        auto& info = r->add<NotifyBtn>(win, "Info", NotificationType::Info);
        TooltipManager::instance().set_tooltip(&info, "Info toast, auto-dismiss");
        r->add<NotifyBtn>(win, "Success", NotificationType::Success);
        r->add<NotifyBtn>(win, "Warning", NotificationType::Warning);
        r->add<NotifyBtn>(win, "Error", NotificationType::Error);
        r->add<Button>("Dismiss All").on_click([&win]() {
            NotificationManager::instance().dismiss_all(win);
        });
    }

    heading("ContextMenu");
    hint("Right-click the host to open a menu");
    {
        auto* host = new MenuHost(win);
        body->append_child(host);
    }

    heading("Overlay");
    hint("Modal dialog with fade + slide animation");
    {
        auto* overlay = new DemoOverlay;
        auto* btn = new Button("Show Overlay");
        btn->on_click([&win, overlay]() {
            if (overlay->is_open()) overlay->close();
            else overlay->show(win);
        });
        TooltipManager::instance().set_tooltip(btn, "Fade + slide modal");
        body->append_child(btn);
    }

    return root;
}

// ---------------------------------------------------------------------
// Theme page (color palette swatches + light/dark toggle)
// ---------------------------------------------------------------------

Widget* build_theme_page(Window& win) {
    auto* root = new DockPanel;
    auto* scroll = new ScrollView;
    root->dock(scroll, Dock::Fill);

    auto* body = new FlexBox(Orientation::Vertical);
    body->set_padding(20.0f);
    body->set_spacing(6.0f);
    body->set_align_cross(FlexCrossAlign::Stretch);
    scroll->set_content(body);

    auto heading = [body](const char* t) {
        auto* l = new Label(t);
        l->set_bold(true).set_align(TextAlignH::Left, TextAlignV::Center)
            .set_margin(Margins{0, 10, 0, 2});
        body->append_child(l);
        return l;
    };

    struct ThemeToggleBtn : Button {
        Window& win;
        bool dark = true;
        ThemeToggleBtn(Window& w) : Button("Switch to Light"), win(w) {
            set_cursor(Cursor::Hand);
        }
        void on_click() override {
            dark = !dark;
            Theme::set(dark ? Theme::make_dark() : Theme::make_light());
            win.invalidate_all();
            set_text(dark ? "Switch to Light" : "Switch to Dark");
        }
    };

    heading("Theme");
    body->append_child(new ThemeToggleBtn(win));

    heading("Palette");
    {
        const Theme& th = Theme::get();
        auto* grid = new WrapPanel;
        grid->set_spacing(8.0f).set_line_spacing(8.0f);
        struct Swatch { const char* name; Color color; };
        const Swatch swatches[] = {
            {"background", th.background},
            {"surface", th.surface},
            {"surface_container", th.surface_container},
            {"surface_container_high", th.surface_container_high},
            {"accent", th.accent},
            {"accent_text", th.accent_text},
            {"accent_hover", th.accent_hover},
            {"text", th.text},
            {"text_secondary", th.text_secondary},
            {"text_disabled", th.text_disabled},
            {"border", th.border},
        };
        for (const auto& sw : swatches) {
            auto* card = new Box(sw.color);
            card->radius(6.0f).set_min_size(Size{120.0f, 44.0f});
            card->add<Label>(sw.name).small(true);
            grid->append_child(card);
        }
        body->append_child(grid);
    }

    heading("Controls follow the theme");
    {
        auto* r = new FlexBox;
        r->set_spacing(10.0f).set_align_cross(FlexCrossAlign::Center);
        r->add<Button>("Primary");
        r->add<Button>("Secondary").set_accent(false);
        auto& slider = r->add<Slider>();
        slider.set_value(50.0f);
        body->append_child(r);
    }

    return root;
}

// ---------------------------------------------------------------------
// Layouts page (Dock / Flex / Wrap / Scroll)
// ---------------------------------------------------------------------

Widget* build_layouts_page() {
    auto* root = new DockPanel;
    auto* scroll = new ScrollView;
    root->dock(scroll, Dock::Fill);

    auto* body = new FlexBox(Orientation::Vertical);
    body->set_padding(20.0f);
    body->set_spacing(6.0f);
    body->set_align_cross(FlexCrossAlign::Stretch);
    scroll->set_content(body);

    auto heading = [body](const char* t) {
        auto* l = new Label(t);
        l->set_bold(true).set_align(TextAlignH::Left, TextAlignV::Center)
            .set_margin(Margins{0, 10, 0, 2});
        body->append_child(l);
        return l;
    };

    auto hint = [body](const char* t) {
        auto* l = new Label(t);
        l->text_role(TextRole::Secondary).small(true)
            .set_align(TextAlignH::Left, TextAlignV::Center);
        body->append_child(l);
        return l;
    };

    const auto colored = [](Widget* parent, const char* text, Color c, f32 w, f32 h) {
        auto* box = new Box(c);
        box->radius(6.0f).set_min_size(Size{w, h});
        box->add<Label>(text).small(true);
        parent->append_child(box);
        return box;
    };

    heading("DockPanel: Left/Top/Right/Bottom/Fill");
    {
        auto* demo = new DockPanel;
        demo->set_min_size(Size{440.0f, 160.0f});
        const Theme& th = Theme::get();
        demo->dock(colored(demo, "Left", th.accent, 70.0f, 0.0f), Dock::Left);
        demo->dock(colored(demo, "Top", th.surface_container_high, 0.0f, 30.0f), Dock::Top);
        demo->dock(colored(demo, "Right", th.accent_hover, 70.0f, 0.0f), Dock::Right);
        demo->dock(colored(demo, "Fill", th.surface_container, 0.0f, 0.0f), Dock::Fill);
        body->append_child(demo);
    }

    heading("FlexBox: SpaceBetween + cross-axis center");
    {
        auto* r = new FlexBox;
        r->set_spacing(8.0f).set_align_main(FlexAlign::SpaceBetween)
            .set_align_cross(FlexCrossAlign::Center);
        const Theme& th = Theme::get();
        colored(r, "A", th.accent, 60.0f, 40.0f);
        colored(r, "B", th.accent_hover, 60.0f, 40.0f);
        colored(r, "C", th.surface_container_high, 60.0f, 40.0f);
        body->append_child(r);
    }

    heading("FlexBox: flex_grow splits leftover space");
    {
        auto* r = new FlexBox;
        r->set_spacing(8.0f);
        const Theme& th = Theme::get();
        auto* fixed = colored(r, "fixed", th.accent, 70.0f, 40.0f);
        (void)fixed;
        colored(r, "grow", th.surface_container_high, 70.0f, 40.0f)->set_flex_grow(1.0f);
        body->append_child(r);
    }

    heading("WrapPanel");
    {
        auto* wrap = new WrapPanel;
        wrap->set_spacing(6.0f).set_line_spacing(6.0f);
        const Theme& th = Theme::get();
        const char* tags[] = {"Dock", "Flex", "Wrap", "Stack", "Grid", "Scroll", "Overlay"};
        for (const char* t : tags) {
            auto* box = new Box(th.surface_container_high);
            box->radius(10.0f).set_min_size(Size{64.0f, 26.0f});
            box->add<Label>(t).small(true);
            wrap->append_child(box);
        }
        body->append_child(wrap);
    }

    heading("ScrollView: nested scrolling");
    {
        auto* outer = new ScrollView;
        outer->set_suggested_height(150.0f);
        auto* stack = new StackPanel(Orientation::Vertical);
        stack->set_spacing(6.0f);
        stack->set_padding(4.0f);
        outer->set_content(stack);
        const Theme& th = Theme::get();
        for (int i = 0; i < 4; ++i) {
            auto* section = new Box(th.surface);
            section->radius(8.0f).padding(6.0f);
            section->add<Label>("Section " + std::to_string(i + 1)).bold(true);
            auto* inner = new ScrollView;
            inner->set_suggested_height(60.0f);
            auto* inner_stack = new StackPanel(Orientation::Vertical);
            inner_stack->set_spacing(4.0f);
            inner->set_content(inner_stack);
            for (int j = 0; j < 6; ++j) {
                inner_stack->add<Label>("  Nested item " + std::to_string(j + 1)).small(true);
            }
            section->append_child(inner);
            stack->append_child(section);
        }
        body->append_child(outer);
    }

    return root;
}

// ---------------------------------------------------------------------
// App shell
// ---------------------------------------------------------------------

Widget* make_playground(Window& win) {
    const Theme& th = Theme::get();

    auto* root = new DockPanel;

    // Sidebar
    auto* sidebar = new Box();
    sidebar->bg_role(ThemeRole::Surface).set_min_size(Size{176.0f, 0.0f});
    root->dock(sidebar, Dock::Left);

    auto* side_stack = new StackPanel(Orientation::Vertical);
    side_stack->set_spacing(4.0f).set_padding(8.0f);
    sidebar->append_child(side_stack);

    auto* title = new Label("Playground");
    title->bold(true).set_align(TextAlignH::Center, TextAlignV::Center)
        .set_text_color(th.accent).set_margin(Margins{0, 0, 0, 6});
    side_stack->append_child(title);

    // Content area: DockPanel holds all pages as overlapping Fill children.
    // Only the active page is visible, and Fill makes it consume the entire
    // region. (A vertical StackPanel would size each page to its content
    // height — a ListView root reports ~84px — leaving the page to shrink.)
    auto* content = new Box;
    root->dock(content, Dock::Fill);

    auto* pages = new DockPanel;
    content->append_child(pages);

    auto* page_widgets = build_widgets_page();
    auto* page_todo = build_todo_page(win);
    auto* page_custom = build_custom_page(win);
    auto* page_feedback = build_feedback_page(win);
    auto* page_theme = build_theme_page(win);
    auto* page_layouts = build_layouts_page();
    pages->dock(page_widgets, Dock::Fill);
    pages->dock(page_todo, Dock::Fill);
    pages->dock(page_custom, Dock::Fill);
    pages->dock(page_feedback, Dock::Fill);
    pages->dock(page_theme, Dock::Fill);
    pages->dock(page_layouts, Dock::Fill);

    // NavItem exposes a click callback now, so page switching needs no subclass.
    auto* app_nav = new AppNav;
    app_nav->pages_host = pages;

    auto link = [&](const char* t, PageId id, Widget* page) {
        auto* li = new NavItem(t, id);
        li->set_on_activate([app_nav, id]() { app_nav->activate(id); });
        li->active = (id == PageId::Widgets);
        side_stack->append_child(li);
        app_nav->entries.push_back({li, page, id});
        page->set_visible(id == PageId::Widgets);
        return li;
    };

    link("Widgets", PageId::Widgets, page_widgets);
    link("Todo", PageId::Todo, page_todo);
    link("Custom", PageId::Custom, page_custom);
    link("Feedback", PageId::Feedback, page_feedback);
    link("Theme", PageId::Theme, page_theme);
    link("Layouts", PageId::Layouts, page_layouts);

    auto* toggle = new Button("Theme: Dark");
    bool dark = true;
    toggle->on_click([&win, &dark, toggle]() {
        dark = !dark;
        Theme::set(dark ? Theme::make_dark() : Theme::make_light());
        win.invalidate_all();
        toggle->set_text(dark ? "Theme: Dark" : "Theme: Light");
    }).set_margin(Margins{0, 8, 0, 0});
    side_stack->append_child(toggle);

    return root;
}

}  // namespace yzk