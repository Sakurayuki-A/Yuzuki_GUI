#include "main_page.hpp"

#include <yuzuki/controls/wrap_panel.hpp>
#include <yuzuki/controls/list_view.hpp>
#include <yuzuki/controls/label.hpp>
#include <yuzuki/controls/button.hpp>
#include <yuzuki/controls/box.hpp>
#include <yuzuki/ui/animation.hpp>

#include <windows.h>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <functional>

namespace yzk {

namespace {

const Color kBg{0x16, 0x16, 0x18};
const Color kPanel{0x20, 0x20, 0x24};

// The demo owns the widget tree: recursive deletion
void delete_tree(Widget* w) {
    if (!w) return;
    for (Widget* c = w->first_child(); c; c = c->next_sibling()) delete_tree(c);
    delete w;
}

size_t count_tree(const Widget* w) {
    size_t n = 1;
    for (const Widget* c = w->first_child(); c; c = c->next_sibling()) n += count_tree(c);
    return n;
}

Label* make_label(const char* text, TextRole role = TextRole::Primary) {
    auto l = new Label(text);
    l->set_text_role(role);
    l->set_small(true);
    l->set_align(TextAlignH::Left, TextAlignV::Center);
    return l;
}

// ===== Scenario: 100k-row virtual list =====
struct BigSource : ListView::DataSource {
    i32 count() const override { return 100000; }
    String text_at(i32 index) const override {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "row %d", index);
        return buf;
    }
};

struct BigDelegate : ListView::RowDelegate {
    void draw(ListView& view, PaintContext& ctx, i32 index, const RectF& row_rect) override {
        ctx.draw_text(view.text_at(index), row_rect,
                      index == view.selected() ? ctx.theme().accent_text : ctx.theme().text,
                      TextAlignH::Left, TextAlignV::Center);
    }
};

// ===== Scenario host: destroys the old tree and stops frame driving on switch =====
struct ScenarioHost {
    Widget* host = nullptr;
    Widget* current = nullptr;
    AnimationSystem::FrameToken token = 0;
    bool driven = false;

    void stop_drive() {
        if (driven) {
            AnimationSystem::instance().stop_frame(token);
            driven = false;
        }
    }
    void swap(std::function<Widget*()> build) {
        stop_drive();
        if (current) {
            host->clear_children();
            delete_tree(current);
            current = nullptr;
        }
        current = build();
        host->append_child(current);
    }
    void drive(std::function<void(f32)> cb) {
        stop_drive();
        token = AnimationSystem::instance().on_frame(std::move(cb));
        driven = true;
    }
};

// ===== Animation storm: a field of manually positioned boxes =====
struct StormField : Widget {
    std::vector<Box*> boxes;
    void perform_layout(const PaintContext* ctx) override {
        // Box positions are set manually by the frame callback; no layout is performed
    }
};

// ===== HUD: samples frame_stats on a timer =====
struct HudPanel : Box {
    Window& win;
    Label* fps_label;
    Label* breakdown_label;
    Label* cmds_label;
    Label* widgets_label;
    u64 last_frames = 0;
    f32 last_total = 0;
    f32 last_layout = 0;
    f32 last_record = 0;
    f32 last_replay = 0;
    u64 last_cmds = 0;
    u64 last_pump = 0;
    u64 last_rects = 0;
    u64 last_full = 0;
    f64 last_poll_ms = 0;

    explicit HudPanel(Window& w)
        : win(w), fps_label(nullptr), breakdown_label(nullptr), cmds_label(nullptr),
          widgets_label(nullptr) {}

    void on_event(Event& e) override {
        if (e.type == EventType::Timer) poll();
        Box::on_event(e);
    }

    void poll() {
        const auto& st = win.frame_stats();
        const f64 now = static_cast<f64>(
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now().time_since_epoch())
                .count());
        char buf[256];
        const f64 elapsed = now - last_poll_ms;
        if (elapsed > 0.0 && st.frames > last_frames) {
            const f64 df = static_cast<f64>(st.frames - last_frames);
            std::snprintf(buf, sizeof(buf), "%.0f FPS", df / (elapsed / 1000.0));
            fps_label->set_text(buf);
            std::snprintf(buf, sizeof(buf), "frame %.2f ms\nlayout %.2f  record %.2f  replay %.2f",
                          (st.total_ms - last_total) / df, (st.layout_ms - last_layout) / df,
                          (st.record_ms - last_record) / df, (st.replay_ms - last_replay) / df);
            breakdown_label->set_text(buf);
            std::snprintf(buf, sizeof(buf), "%llu cmds/frame",
                          static_cast<unsigned long long>((st.commands - last_cmds) / df));
            cmds_label->set_text(buf);
            if (FILE* f = std::fopen("perf_demo.log", "a")) {
                std::fprintf(f,
                             "fps=%.0f frame=%.2f layout=%.2f record=%.2f replay=%.2f cmds=%llu "
                             "pump/s=%.0f rects/f=%.1f full=%u\n",
                             df / (elapsed / 1000.0), (st.total_ms - last_total) / df,
                             (st.layout_ms - last_layout) / df, (st.record_ms - last_record) / df,
                             (st.replay_ms - last_replay) / df,
                             static_cast<unsigned long long>((st.commands - last_cmds) / df),
                             static_cast<double>(st.pump_calls - last_pump) / (elapsed / 1000.0),
                             static_cast<f64>(st.rendered_rects - last_rects) / df,
                             static_cast<u32>(st.full_frames - last_full));
                std::fclose(f);
            }
        }
        last_frames = st.frames;
        last_total = st.total_ms;
        last_layout = st.layout_ms;
        last_record = st.record_ms;
        last_replay = st.replay_ms;
        last_cmds = st.commands;
        last_pump = st.pump_calls;
        last_rects = st.rendered_rects;
        last_full = st.full_frames;
        last_poll_ms = now;
    }

    void set_widget_count(Widget* root) {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%zu widgets", count_tree(root));
        widgets_label->set_text(buf);
    }
};

struct Page {
    Window& win;
    ScenarioHost host;
    HudPanel* hud = nullptr;
    Button* scenario_buttons[6] = {};

    explicit Page(Window& w) : win(w) {}

    // ===== Scenario builders =====
    Widget* build_baseline() {
        auto body = new FlexBox(Orientation::Vertical);
        body->set_padding(24.0f);
        body->set_spacing(10.0f);
        auto title = new Label("Baseline - idle window");
        title->set_bold(true);
        title->set_align(TextAlignH::Left, TextAlignV::Center);
        body->append_child(title);
        body->append_child(
            make_label("Only the HUD repaints every 500 ms (partial frames).", TextRole::Secondary));
        return body;
    }

    Widget* build_grid() {
        auto wrap = new WrapPanel;
        wrap->set_spacing(6.0f);
        wrap->set_line_spacing(6.0f);
        const Color palette[] = {Color{0x3b, 0x3b, 0x40}, Color{0x4a, 0x4a, 0x52},
                                 Color{0x5a, 0x5a, 0x64}, Color{0x66, 0x66, 0x70}};
        for (int i = 0; i < 2000; ++i) {
            auto cell = new Box;
            cell->set_min_size(Size{20.0f, 20.0f});
            cell->set_radius(4.0f);
            cell->set_bg(palette[i % 4]);
            wrap->append_child(cell);
        }
        // Light up one random cell per frame: cost of a 2000-widget tree + single-dirty-region partial frames
        host.drive([wrap](f32) {
            static int i = 0;
            auto it = wrap->first_child();
            for (int k = 0; k < i && it; ++k) it = it->next_sibling();
            if (it) {
                static_cast<Box*>(it)->set_bg(Color{0x5a, 0x9c, 0xff});
                i = (i + 1) % 2000;
            }
        });
        return wrap;
    }

    Widget* build_shadows() {
        auto wrap = new WrapPanel;
        wrap->set_spacing(14.0f);
        wrap->set_line_spacing(14.0f);
        for (int i = 0; i < 90; ++i) {
            auto card = new Box;
            card->set_min_size(Size{76.0f, 44.0f});
            card->set_radius(8.0f);
            card->set_bg(Color{0x2c, 0x2c, 0x32});
            card->set_shadow(14.0f, 5.0f);
            card->set_shadow_color(Color{0xff, 0xff, 0xff, 0x37});
            wrap->append_child(card);
        }
        return wrap;
    }

    Widget* build_list() {
        static BigSource source;
        static BigDelegate delegate;
        auto list = new ListView;
        list->set_data_source(&source);
        list->set_row_delegate(&delegate);
        list->set_row_height(26.0f);
        // Time-based driving: constant 60px/s scroll, independent of frame rate
        host.drive([list](f32 now_ms) {
            const f32 max_y = static_cast<f32>(list->count()) * 26.0f - list->height();
            list->set_scroll_y(std::fmod(now_ms * 0.06f, max_y));
        });
        return list;
    }

    Widget* build_storm() {
        auto field = new StormField;
        for (int i = 0; i < 200; ++i) {
            auto box = new Box;
            box->set_min_size(Size{22.0f, 22.0f});
            box->set_radius(5.0f);
            box->set_bg(Color{0x4c, 0xaf, 0x50});
            field->boxes.push_back(box);
            field->append_child(box);
        }
        host.drive([field](f32 now_ms) {
            const f32 t = now_ms * 0.001f;
            for (size_t i = 0; i < field->boxes.size(); ++i) {
                const f32 x = static_cast<f32>(i % 40) * 30.0f +
                              std::sin(t * 3.0f + static_cast<f32>(i) * 0.35f) * 14.0f;
                const f32 y = static_cast<f32>(i / 40) * 30.0f +
                              std::cos(t * 2.6f + static_cast<f32>(i) * 0.17f) * 11.0f;
                field->boxes[i]->set_bounds(RectF::make(x, y, 22.0f, 22.0f));
            }
        });
        return field;
    }

    Widget* build_full_repaint() {
        auto body = build_baseline();
        host.drive([this](f32) { win.invalidate_all(); });
        return body;
    }

    void switch_scenario(int id, Button* active_btn) {
        for (Button* b : scenario_buttons)
            if (b) b->set_accent(b == active_btn);
        switch (id) {
            case 0: host.swap([this] { return build_baseline(); }); break;
            case 1: host.swap([this] { return build_grid(); }); break;
            case 2: host.swap([this] { return build_shadows(); }); break;
            case 3: host.swap([this] { return build_list(); }); break;
            case 4: host.swap([this] { return build_storm(); }); break;
            case 5: host.swap([this] { return build_full_repaint(); }); break;
        }
        if (hud) hud->set_widget_count(host.host);
    }
};

struct ScenarioButton : Button {
    Page& page;
    int id;
    ScenarioButton(Page& p, int scenario_id, const String& text)
        : Button(text), page(p), id(scenario_id) {}
    void on_click() override { page.switch_scenario(id, this); }
};

}  // namespace

Widget* make_perf_demo_page(Window& window, int initial_scenario) {
    static Page* page = nullptr;
    if (!page) page = new Page(window);
    Page& P = *page;

    auto root = new DockPanel;
    auto bg = new Box;
    bg->set_bg(kBg);
    root->dock(bg, Dock::Fill);

    // Sidebar: scenario switcher + HUD
    auto sidebar = new Box;
    sidebar->set_bg(kPanel);
    sidebar->set_radius(0.0f);
    sidebar->set_padding(16.0f);
    sidebar->set_min_size(Size{240.0f, 0.0f});
    sidebar->set_max_size(Size{240.0f, 10000.0f});
    root->dock(sidebar, Dock::Left);

    auto sidebar_col = new FlexBox(Orientation::Vertical);
    sidebar_col->set_spacing(8.0f);
    sidebar->append_child(sidebar_col);

    auto title = new Label("Performance");
    title->set_bold(true);
    title->set_align(TextAlignH::Left, TextAlignV::Center);
    sidebar_col->append_child(title);
    sidebar_col->append_child(make_label("scenarios:", TextRole::Secondary));

    auto hud = new HudPanel(window);
    P.hud = hud;

    const char* names[] = {"baseline (idle)",     "2000-widget grid",
                           "90 shadow cards",     "100k-row list autoscroll",
                           "200 animated boxes",  "full repaint every frame"};
    Button* buttons[6] = {};
    for (int i = 0; i < 6; ++i) {
        auto b = new ScenarioButton(P, i, names[i]);
        buttons[i] = b;
        P.scenario_buttons[i] = b;
        sidebar_col->append_child(b);
    }

    auto spacer = new Box;
    spacer->set_flex_grow(1.0f);
    sidebar_col->append_child(spacer);

    hud->set_bg(Color{0x18, 0x18, 0x1c});
    hud->set_radius(8.0f);
    hud->set_padding(10.0f);
    auto hud_col = new FlexBox(Orientation::Vertical);
    hud_col->set_spacing(4.0f);
    hud->append_child(hud_col);
    auto hud_title = new Label("Frame stats");
    hud_title->set_bold(true);
    hud_title->set_align(TextAlignH::Left, TextAlignV::Center);
    hud_col->append_child(hud_title);
    hud->fps_label = make_label("--");
    hud_col->append_child(hud->fps_label);
    hud->breakdown_label = make_label("--");
    hud_col->append_child(hud->breakdown_label);
    hud->cmds_label = make_label("--");
    hud_col->append_child(hud->cmds_label);
    hud->widgets_label = make_label("--");
    hud_col->append_child(hud->widgets_label);
    sidebar_col->append_child(hud);

    // Scenario container
    auto content = new DockPanel;
    root->dock(content, Dock::Fill);
    P.host.host = content;

    // Initial scenario + HUD timer sampling
    P.host.swap([&P] { return P.build_baseline(); });
    buttons[0]->set_accent(true);
    if (initial_scenario >= 0 && initial_scenario <= 5) {
        P.switch_scenario(initial_scenario, buttons[initial_scenario]);
    }
    hud->set_widget_count(content);
    window.start_timer(hud, 500);

    return root;
}

}  // namespace yzk