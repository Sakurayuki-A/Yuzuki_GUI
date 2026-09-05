#include <yuzuki/yuzuki.hpp>

// Chat UI Cookbook
//
// 覆盖 Design System 语义:
//   color : Accent(自己气泡 bg+白字) / SurfaceContainerLow(他人气泡) / Secondary(时间)
//   text  : 消息正文 Primary / 顶栏标题 bold / 时间 small+Secondary
// 布局:DockPanel 顶栏 Top + 消息列表 Fill + 输入栏 Bottom。
// 交互:输入 + Send 或回车 → 自绘气泡追加到底部,ScrollView 滚到底。

using namespace yzk;

namespace {

// 消息气泡:自绘 Widget,自己靠右 / 他人靠左
class Bubble : public Widget {
public:
    Bubble(bool mine, String text) : mine_(mine), text_(std::move(text)) {}

protected:
    Size measure_impl(Size available, const PaintContext* ctx) override {
        const f32 w = available.width > 0.0f ? available.width : 500.0f;
        const f32 max_bubble = w * 0.72f;
        const f32 pad = 12.0f;
        const Size s = ctx->measure_text(text_, false, max_bubble - pad * 2.0f, true);
        cached_h_ = s.height + pad * 2.0f;
        return Size{0.0f, cached_h_};
    }

    void paint_impl(PaintContext& ctx) override {
        const Theme& t = ctx.theme();
        const Color bg = mine_ ? t.accent : t.surface_container_low;
        const Color fg = mine_ ? t.accent_text : t.text;

        const f32 w = bounds_.width();
        const f32 max_bubble = w * 0.72f;
        const f32 pad = 12.0f;
        const f32 text_w = max_bubble - pad * 2.0f;
        const Size s = ctx.measure_text(text_, false, text_w, true);
        const f32 bh = s.height + pad * 2.0f;
        const f32 bw = (s.width + pad * 2.0f) > max_bubble ? max_bubble : (s.width + pad * 2.0f);
        const float x = mine_ ? bounds_.right - pad - bw : bounds_.left + pad;
        const RectF bubble = RectF::make(x, bounds_.top, bw, bh);
        ctx.fill_rounded(bubble, bg, t.radius_md);
        const RectF text_area = bubble.inflated(-pad, -pad);
        ctx.draw_text(text_, text_area, fg, TextAlignH::Left, TextAlignV::Top, true);
    }

private:
    bool mine_;
    String text_;
    f32 cached_h_ = 0.0f;
};

// 输入栏容器:外壳画圆角输入框,TextBox 透明占上部,底部内部栏放 Send。
class Composer : public Layout {
public:
    TextBox* input;
    Button* send;

    Composer() {
        TextBoxConfig cfg;
        cfg.mode = TextBoxMode::MultiLine;
        cfg.max_length = 4096;
        cfg.height = 96.0f;
        cfg.enter_submits = true;
        cfg.transparent = true;  // 外壳(本类)统一画圆角 + 边框
        input = new TextBox(String(), cfg);
        input->set_placeholder("Type a message...");
        send = new Button("Send");
        // Button 有 icon 时 measure 会 + icon_size+6,min_width 要减去那部分
        send->set_accent(true).set_icon(IconId::PaperPlane).set_icon_size(16.0f)
             .set_min_width(70.0f);
        append_child(input);
        append_child(send);
    }

protected:
    // 内栏:Send 用标准控件高度(control_height==32),两侧各留 4px 外边距
    inline static const f32 kBarH = 40.0f;
    inline static const f32 kBarPad = 4.0f;

    void paint_impl(PaintContext& ctx) override {
        const Theme& t = ctx.theme();
        // 输入框外壳:整个 Composer 由一个圆角输入框代表
        ctx.fill_rounded(bounds_, t.surface, t.control_radius);
        ctx.draw_border(bounds_, t.border, t.border_width, t.control_radius);
        // bar 顶部分隔线(在输入框内部)
        const f32 bar_top = bounds_.bottom - padding() - kBarH;
        ctx.draw_line(Point{bounds_.left + 1.0f, bar_top + 0.5f},
                      Point{bounds_.right - 1.0f, bar_top + 0.5f}, t.border, 1.0f);
        Layout::paint_impl(ctx);
    }

    Size measure_content(Size available, const PaintContext* ctx) override {
        Size s;
        s.width = available.width;
        s.height = input->measure(Size{available.width, 1e7f}, ctx).height + kBarH;
        return s;
    }

    void arrange_content(const RectF& area, const PaintContext* ctx) override {
        const Size ss = send->measure(Size{1e7f, kBarH}, ctx);
        // area 是本地坐标(0..w);外壳总高 = area.height + 2*padding。
        // 分隔线 y = area.height + padding - kBarH;底边 y = area.height + 2*padding。
        // Send 垂直居中于这段区块(分隔线 → 外壳下缘,含底下 padding)。
        const f32 sep = area.height() + padding() - kBarH;
        const f32 bottom = area.height() + padding() * 2.0f;
        input->set_bounds(RectF::make(area.left, area.top, area.width(),
                                      area.height() - kBarH));
        send->set_bounds(RectF::make(area.right - ss.width - kBarPad,
                                     sep + (bottom - sep - ss.height) * 0.5f,
                                     ss.width, ss.height));
        input->perform_layout(ctx);
        send->perform_layout(ctx);
    }
};

} // namespace

int main() {
    auto& app = Application::instance();
    Window win("Chat - Cookbook", 560, 480);
    win.create();
    win.backend().add_font_file("LexendDeca-Regular.ttf");

    auto* shell = new DockPanel;
    win.set_root(shell);

    // 顶栏
    auto* topbar = new Box;
    topbar->set_bg(Theme::get().surface).set_padding(12.0f);
    shell->dock(topbar, Dock::Top);
    auto* top_col = new FlexBox(Orientation::Vertical);
    top_col->set_spacing(2.0f);
    topbar->append_child(top_col);
    auto* title = new Label("Codex Chat");
    title->set_bold(true).set_align(TextAlignH::Left, TextAlignV::Center);
    top_col->append_child(title);
    auto* status = new Label("RichText lite: **bold**, ~highlight~, [links](open:...). Ctrl+Z 可撤销输入。");
    status->set_rich_text(true).set_small(true).set_text_role(TextRole::Secondary)
           .set_align(TextAlignH::Left, TextAlignV::Center);
    top_col->append_child(status);

    // 输入栏(Bottom)必须先于 Fill,否则 Fill 会吃掉整个剩余矩形盖住输入栏
    auto* composer = new Composer;
    composer->set_padding(10.0f);
    shell->dock(composer, Dock::Bottom);

    auto* input = composer->input;
    auto* send = composer->send;

    // 消息列表(Fill):最后的 Fill 只占剩下(Top 与 Bottom 之间)的区域
    auto* scroll = new ScrollView;
    shell->dock(scroll, Dock::Fill);

    auto* msgs = new FlexBox;
    msgs->set_direction(Orientation::Vertical)
         .set_spacing(10.0f)
         .set_align_cross(FlexCrossAlign::Stretch)
         .set_padding(12.0f);
    scroll->set_content(msgs);

    // 5.3.2 RichText lite 演示:一条带强调 + 链接的消息(点击链接在顶栏回显)
    auto* announce = new Label(
        "**Welcome** — tap ~this link~ to see the click callback: "
        "[open docs](open:https://yuzuki.dev/docs)");
    announce->set_rich_text(true)
             .set_align(TextAlignH::Left, TextAlignV::Center)
             .on_span_click([status](const String& action) {
                 status->set_text("link clicked: " + action);
             });
    msgs->append_child(announce);

    msgs->append_child(new Bubble(false, "Hey! Set up the new chat window."));
    msgs->append_child(new Bubble(true, "Nice, using Yuzuki controls now."));
    msgs->append_child(new Bubble(false, "The ScrollView keeps everything tidy."));

    // 发送:追加气泡 + 滚到底
    auto send_msg = [&win, input, msgs, scroll]() {
        String t = input->text();
        if (t.empty()) return;
        input->set_text(String());
        msgs->append_child(new Bubble(true, std::move(t)));
        win.invalidate_all();
        scroll->scroll_to_bottom();  // 下帧 layout 出真实高度后再落底
    };
    input->set_on_commit(send_msg);
    send->set_on_click(send_msg);

    win.show();
    return app.run();
}