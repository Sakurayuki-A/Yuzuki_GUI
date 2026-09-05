#include <yuzuki/yuzuki.hpp>

// Settings 页 Cookbook — 5.4 后由 TabControl 组织 4 个设置分页。
//
// 覆盖 Design System 语义(用 token 而非裸色):
//   color : SurfaceContainerLow(卡片) / TextSecondary(分组小标) / accent(主按钮+开关开态)
//   text  : 页面标题 bold / 分组标题 bold / 说明 small+Secondary
//   radius: control_radius(控件) / radius_md(卡片)
//   spacing: 分组 18 / 组内同行 12
// 布局:DockPanel(内容 Dock::Fill 的 TabControl + 底部按钮 Dock::Bottom)。
// TabControl 本身消费 surface(标签条)/border(分隔线)/accent(选中下划线)/control_height(标签条高)。
// 交互:Tab 点击/←→ 切换;开关即时改主题色调、ComboBox 选强调色、Slider 调全局字号。
// 5.4 三原则自检:消费 design token ✓;cookbook 用例(本节)✓;示例使用(cookbook_settings)✓。

using namespace yzk;

namespace {

// 卡片:SurfaceContainerLow 背景 + 内边距 + 圆角。
class Card : public Box {
public:
    Card() {
        set_bg_role(ThemeRole::SurfaceContainerLow)
            .set_radius(Theme::get().radius_md)
            .set_padding(16.0f);
    }
};

// 一行:说明(flex grow)+ 交互控件(右对齐)。
class SettingRow {
public:
    FlexBox* root;
    FlexBox* labels;
    Label* title;
    Label* desc;
    Widget* control;

    SettingRow(const char* t, const char* d, Widget* c) {
        root = new FlexBox;
        root->set_align_cross(FlexCrossAlign::Center).set_spacing(12.0f);

        labels = new FlexBox;
        labels->set_direction(Orientation::Vertical).set_spacing(2.0f);

        title = new Label(t);
        desc = new Label(d);
        desc->set_small(true).set_text_role(TextRole::Secondary);

        labels->append_child(title);
        labels->append_child(desc);
        labels->set_flex_grow(1.0f);

        root->append_child(labels);
        root->append_child(c);
        control = c;
    }
};

Label* section_title(const char* t) {
    auto* l = new Label(t);
    l->set_bold(true);
    return l;
}

} // namespace

int main() {
    auto& app = Application::instance();

    Window win("Settings - Cookbook", 900, 620);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");

    // ---- 骨架:DockPanel,TabControl(Dock::Fill)+ 底部按钮(Dock::Bottom) ----
    auto* shell = new DockPanel;
    win.set_root(shell);

    auto* tabs = new TabControl;
    tabs->add_tab("Appearance", new FlexBox).add_tab("General", new FlexBox)
        .add_tab("Notifications", new FlexBox).add_tab("About", new FlexBox);
    shell->dock(tabs, Dock::Fill);

    // ---- Appearance 分页 ----
    {
        auto* page = static_cast<FlexBox*>(tabs->page(0));
        page->set_direction(Orientation::Vertical)
             .set_spacing(18.0f)
             .set_align_cross(FlexCrossAlign::Stretch)
             .set_padding(24.0f);

        auto* appearance_card = new Card;
        auto* a_col = new FlexBox;
        a_col->set_direction(Orientation::Vertical)
              .set_spacing(12.0f)
              .set_align_cross(FlexCrossAlign::Stretch);
        appearance_card->append_child(a_col);

        a_col->append_child(section_title("Appearance"));
        auto* accent_combo = new ComboBox(std::vector<String>{"Indigo", "Emerald", "Rose", "Slate"});
        accent_combo->set_width(180.0f).set_selected_index(0);
        SettingRow accent_row("Accent color", "Pick the brand accent.", accent_combo);
        a_col->append_child(accent_row.root);

        auto* font_slider = new Slider;
        font_slider->set_range(0.8f, 2.0f).set_value(1.0f);
        SettingRow font_row("Base font size", "Sets the base font scale.", font_slider);
        a_col->append_child(font_row.root);

        auto* compact_toggle = new ToggleSwitch;
        SettingRow compact_row("Compact mode", "Tighter vertical rhythm.", compact_toggle);
        a_col->append_child(compact_row.root);

        page->append_child(appearance_card);
    }

    // ---- General 分页 ----
    {
        auto* page = static_cast<FlexBox*>(tabs->page(1));
        page->set_direction(Orientation::Vertical)
             .set_spacing(18.0f)
             .set_align_cross(FlexCrossAlign::Stretch)
             .set_padding(24.0f);

        auto* general_card = new Card;
        auto* g_col = new FlexBox;
        g_col->set_direction(Orientation::Vertical)
              .set_spacing(12.0f)
              .set_align_cross(FlexCrossAlign::Stretch);
        general_card->append_child(g_col);

        g_col->append_child(section_title("General"));
        auto* autosave = new CheckBox("Save on close");
        SettingRow autosave_row("Auto save", "Persist on window close.", autosave);
        g_col->append_child(autosave_row.root);

        auto* cache_spin = new SpinBox(5.0, 0, 60, 1);
        cache_spin->set_decimals(1);
        SettingRow cache_row("Cache (min)", "Minutes of cache to keep.", cache_spin);
        g_col->append_child(cache_row.root);

        page->append_child(general_card);
    }

    // ---- Notifications 分页 ----
    {
        auto* page = static_cast<FlexBox*>(tabs->page(2));
        page->set_direction(Orientation::Vertical)
             .set_spacing(18.0f)
             .set_align_cross(FlexCrossAlign::Stretch)
             .set_padding(24.0f);

        auto* notif_card = new Card;
        auto* n_col = new FlexBox;
        n_col->set_direction(Orientation::Vertical)
              .set_spacing(12.0f)
              .set_align_cross(FlexCrossAlign::Stretch);
        notif_card->append_child(n_col);

        n_col->append_child(section_title("Notifications"));
        auto* sound_toggle = new ToggleSwitch;
        sound_toggle->set_checked(true);
        SettingRow sound_row("Sound", "Play a sound for notifications.", sound_toggle);
        n_col->append_child(sound_row.root);

        page->append_child(notif_card);
    }

    // ---- About 分页 ----
    {
        auto* page = static_cast<FlexBox*>(tabs->page(3));
        page->set_direction(Orientation::Vertical)
             .set_spacing(18.0f)
             .set_align_cross(FlexCrossAlign::Stretch)
             .set_padding(24.0f);

        auto* about_card = new Card;
        auto* ab_col = new FlexBox;
        ab_col->set_direction(Orientation::Vertical)
               .set_spacing(8.0f)
               .set_align_cross(FlexCrossAlign::Stretch);
        about_card->append_child(ab_col);

        ab_col->append_child(section_title("About"));
        auto* about_label = new Label("Yuzuki v0.3.0 - settings with TabControl.");
        about_label->set_small(true).set_text_role(TextRole::Secondary);
        ab_col->append_child(about_label);

        page->append_child(about_card);
    }

    // ---- 底部 Save ----
    auto* footer_row = new FlexBox;
    footer_row->set_align_main(FlexAlign::End).set_spacing(8.0f).set_padding(12.0f);
    auto* cancel_btn = new Button("Cancel");
    cancel_btn->set_accent(false).set_min_width(90.0f);
    auto* save_btn = new Button("Save changes");
    save_btn->set_accent(true).set_min_width(120.0f);
    footer_row->append_child(cancel_btn);
    footer_row->append_child(save_btn);
    shell->dock(footer_row, Dock::Bottom);

    win.show();
    return app.run();
}