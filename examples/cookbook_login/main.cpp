#include <yuzuki/yuzuki.hpp>

// Login 页 Cookbook
//
// 覆盖 Design System 语义:
//   color : SurfaceContainer(外层卡片) / accent(主按钮+焦点) / SurfaceContainerLow(输入行)
//   text  : 页标题 bold / 说明 small+Secondary / 错误 TextSecondary
//   radius: control_radius(输入+按钮) / radius_md(卡片)
// 布局:DockPanel(整窗)+ FlexBox(垂直)+ 卡片;输入行 FlexBox(Horizontal)。
// 交互:回车提交、提交后禁用按钮模拟请求、错误提示。

using namespace yzk;

namespace {

// 卡片:SurfaceContainerLow 背景 + 内边距 + 圆角。
class Card : public Box {
public:
    Card() {
        set_bg_role(ThemeRole::SurfaceContainerLow)
            .set_radius(Theme::get().radius_md)
            .set_padding(20.0f);
    }
};

// 表单标签(label 上方)
class FieldRow {
public:
    FlexBox* root;
    Label* label;
    Widget* field;

    FieldRow(const char* text, Widget* f) : field(f) {
        root = new FlexBox;
        root->set_direction(Orientation::Vertical).set_spacing(6.0f);
        label = new Label(text);
        label->set_small(true).set_text_role(TextRole::Secondary);
        root->append_child(label);
        root->append_child(f);
    }
};

} // namespace

int main() {
    auto& app = Application::instance();
    Window win("Login - Cookbook", 420, 440);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");

    auto* shell = new DockPanel;
    win.set_root(shell);
    // 居中:横纵都放一个 Padding 让卡片浮中
    auto* pad = new FlexBox;
    pad->set_direction(Orientation::Horizontal)
       .set_align_main(FlexAlign::Center)
       .set_align_cross(FlexCrossAlign::Center)
       .set_padding(40.0f);
    shell->dock(pad, Dock::Fill);
    auto* card = new Card;
    card->set_min_size(Size{320.0f, 0.0f});
    pad->append_child(card);

    auto* board = new FlexBox;
    board->set_direction(Orientation::Vertical)
          .set_spacing(14.0f)
          .set_align_cross(FlexCrossAlign::Stretch);
    card->append_child(board);

    // 标题
    auto* title = new Label("Welcome back");
    title->set_bold(true).set_align(TextAlignH::Left, TextAlignV::Center);
    board->append_child(title);

    auto* subtitle = new Label("Sign in to continue");
    subtitle->set_small(true).set_text_role(TextRole::Secondary)
            .set_align(TextAlignH::Left, TextAlignV::Center);
    board->append_child(subtitle);

    // 用户名 / 密码
    auto* user_field = new TextBox(String(), TextBoxConfig{});
    user_field->set_placeholder("Username");
    FieldRow user_row("Username", user_field);
    board->append_child(user_row.root);

    auto* pass_field = new TextBox(String(), TextBoxConfig{TextBoxMode::Password});
    pass_field->set_placeholder("Password");
    FieldRow pass_row("Password", pass_field);
    board->append_child(pass_row.root);

    // 错误提示(默认隐藏,提交后显)
    auto* error = new Label("Invalid username or password");
    error->set_small(true).set_text_role(TextRole::Secondary)
          .set_align(TextAlignH::Left, TextAlignV::Center)
          .set_visible(false);
    board->append_child(error);

    // 主按钮
    auto* login_btn = new Button("Login");
    login_btn->set_accent(true).set_min_width(140.0f);
    login_btn->set_flex_grow(1.0f);
    auto* footer = new FlexBox;
    footer->set_align_main(FlexAlign::End);
    footer->append_child(login_btn);
    board->append_child(footer);

    // 提交逻辑:回车提交 + 点按钮提交
    auto submit = [&win, user_field, pass_field, error, login_btn]() {
        const bool empty = user_field->text().empty() || pass_field->text().empty();
        if (empty) {
            error->set_visible(true);
            return;
        }
        error->set_visible(false);
        login_btn->set_enabled(false);           // 模拟请求中
    };
    user_field->set_on_commit(submit);
    pass_field->set_on_commit(submit);
    login_btn->set_on_click(submit);

    // 初始焦点给用户名
    win.set_focus(user_field);

    win.show();
    return app.run();
}