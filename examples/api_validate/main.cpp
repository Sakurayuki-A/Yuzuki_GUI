// api_validate:按 docs/API.md 逐节照写的验证程序(新手视角)。
// 目的:把 API.md 每个代码块变成可编译运行的真实代码,若有卡点即 DX bug。
// 只需一行 include——yuzuki.hpp 导出全部公共类型(已修复曾缺 Icon/animation)。
#include <yuzuki/yuzuki.hpp>

#include <string>
#include <vector>

using namespace yzk;

// MySource:文档 §3 说 "实现 count()/text_at(i32)"
class MySource : public ListView::DataSource {
public:
    i32 count() const override { return 5; }
    String text_at(i32 index) const override {
        return "Row " + std::to_string(index);
    }
};

// Gauge:文档 §9 自绘控件
class Gauge : public Widget {
public:
    void paint_impl(PaintContext& ctx) override {
        const Theme& t = ctx.theme();
        ctx.fill_rounded(bounds(), t.track, t.control_radius);
        ctx.fill_rounded(RectF::make(bounds().left, bounds().top,
                                     bounds().width() * value_, bounds().height()),
                         t.accent, t.control_radius);
        ctx.draw_text(label(), bounds(), t.text);
    }
    f32 value_ = 0.6f;

private:
    // DX bug #2:文档写 sprintf_value(),无此函数;paint.hpp 也无它。
    String label() const {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%.1f", value_);
        return buf;
    }
};

// MyDialog:文档 §7 模态对话框
class MyDialog : public Overlay {
public:
    MyDialog() {
        set_panel_rect(RectF::make(300, 120, 320, 180));
    }
};

int main() {
    auto& app = Application::instance();

    Window win("API Validation", 640, 560);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");

    auto* root = new DockPanel;
    auto* col = new StackPanel(Orientation::Vertical);
    col->set_spacing(12.0f).set_padding(24.0f);
    root->dock(col, Dock::Fill);
    win.set_root(root);

    // §1:最小窗口骨架(标题 Label + DockPanel 都验证了)
    auto& title = col->add<Label>("Yuzuki API Reference - validation");
    title.set_bold(true).set_small(false);

    // §2:布局参数
    auto* row = new StackPanel(Orientation::Horizontal);
    row->set_spacing(8.0f).set_margin(Margins{0, 4, 0, 4});
    col->append_child(row);

    auto& b1 = row->add<Button>("OK");
    b1.set_min_width(120.0f).set_flex_grow(1.0f);
    auto& b2 = row->add<Button>("Cancel");
    b2.set_min_width(90.0f);

    // §3:文本 / 标题
    auto& label = col->add<Label>("Title");
    label.set_bold(true).set_small(false).set_text_role(TextRole::Secondary);
    label.set_align(TextAlignH::Left, TextAlignV::Center);

    // §3:按钮 / 图标按钮
    auto& go = col->add<Button>("Go");
    go.set_icon(IconId::PaperPlane).set_icon_size(18);
    go.set_accent(false);
    go.set_on_click([&win]() {
        auto* dlg = new MyDialog;
        dlg->show(win);
    });

    // §3:输入:文本 / 数字切换
    auto& box = col->add<TextBox>(String(), TextBoxConfig{});
    box.set_placeholder("Add note...");
    box.set_on_commit([]() {});

    auto& spin = col->add<SpinBox>(0.0, -100, 100, 1);
    spin.set_decimals(2);
    spin.set_on_changed([](f64) {});

    // §3:选择:复选 / 单选 / 开关 / 下拉
    col->add<CheckBox>("Remember me").set_on_toggled([](bool) {});
    col->add<RadioButton>("A");                          // 互斥组
    col->add<RadioButton>("B").set_checked(true);
    col->add<ToggleSwitch>().set_on_toggled([](bool) {});

    auto& combo = col->add<ComboBox>(std::vector<String>{"x", "y", "z"});
    combo.set_selected_index(0).set_on_changed([](i32) {});

    // §3:滑动 / 进度
    auto& slider = col->add<Slider>();
    slider.set_range(0.f, 100.f).set_value(50.f)
       .set_on_changed([](f32) {});
    col->add<ProgressBar>().set_value(0.6f);
    col->add<ProgressBar>().set_indeterminate(true);

    // §3:列表
    auto& list = col->add<ListView>();
    list.set_items(std::vector<String>{"a", "b", "c"});
    list.set_on_selected([](i32) {});
    list.set_on_activate([](i32) {});
    list.set_data_source(new MySource);   // 大列表按需渲染

    // §3:滚动
    auto* page = new StackPanel(Orientation::Vertical);
    auto* scroll = new ScrollView;
    scroll->set_content(page);
    col->append_child(scroll);

    // §3:图片 / 图标
    auto& img = col->add<Image>();
    img.load_from_file(win, "testimg.jpg");
    img.set_scale_mode(ImageScaleMode::Contain);

    col->add<Icon>(IconId::Gear, 18).set_color(Color{0xFF, 0x0, 0x0});

    // §4:交互 — 回调/焦点/定时器(§4 的 focus/timer 用到才写;此处演示回调即足够)

    // §5:外观 — 主题 token + Box 卡片
    Theme::set(Theme::make_dark());      // 或 make_light()
    const Theme& t = Theme::get();

    auto& card = col->add<Box>();
    card.set_bg_role(ThemeRole::SurfaceContainerLow)
         .set_radius(t.control_radius)
         .set_padding(16.f);

    auto& box_card = col->add<Box>();
    box_card.set_border(1.0f, t.border)
            .set_shadow(t.shadow_blur_floating, t.shadow_offset_floating)
            .set_radius(t.control_radius);

    // §6:动画
    auto& anim_target = col->add<Button>("Animate");
    anim_target.set_transition(200.0f);
    anim_target.set_opacity(0.7f);
    anim_target.set_scale(1.1f, 1.1f);

    auto tok = AnimationSystem::instance().tween(0.0f, 1.0f, 280.0f, Easing::OutCubic,
        [&anim_target](f32 v) { anim_target.set_opacity(0.3f + 0.6f * v); });
    AnimationSystem::instance().finish_tween(tok);

    // §7:浮层(MyDialog 改由 Go 按钮触发;ContextMenu/Notification/Tooltip 在此绑定)
    auto* menu = new ContextMenu;
    menu->add_item("Copy", []() {});
    menu->add_item("Paste", []() {});
    menu->add_separator();
    menu->add_item("Delete", []() {});
    win.set_context_menu(menu);

    NotificationManager::instance().show(win, "Title", "Message",
                                         NotificationType::Info, 3200.0f);
    TooltipManager::instance().set_tooltip(&go, "Save it");

    // §8:调试工具
    auto* overlay = new DebugOverlay;
    win.set_debug_overlay(overlay);
    auto* inspector = new WidgetInspector;
    win.set_widget_inspector(inspector);

    // §9:自绘控件
    auto& gauge = col->add<Gauge>();
    gauge.set_min_size(Size{160.0f, 16.0f});

    // §10:无边框/拖拽标题栏(注释掉以免影响目验;单独 uncomment)
    // win.set_borderless(true);
    // win.set_caption(caption_row);

    win.show();
    return app.run();
}