#include "main_page.hpp"
#include "components.hpp"

using namespace yzk;

// Three-pane Codex layout:
//   root → sidebar (Left) + main (Fill); main → topbar (Top) + composer (Bottom) + messages (Fill)
Widget* make_codex_page() {
    // ===== Sidebar =====
    auto sidebar = new Pane(codex::SidebarBg);
    sidebar->set_min_size(Size{264.0f, 0.0f});  // min-size fixes the sidebar width
    sidebar->set_border_right(1.0f, codex::Border);

    auto side_dock = new DockPanel;
    sidebar->set_content(side_dock);

    // Sidebar header: Codex title + New button
    auto header = new Pane(codex::SidebarBg, 52.0f);
    side_dock->dock(header, Dock::Top);
    auto head_row = new DockPanel;
    head_row->set_padding(12.0f);
    header->set_content(head_row);
    auto title = new Label("Codex");
    title->set_bold(true);
    title->set_align(TextAlignH::Left, TextAlignV::Center);
    auto new_btn = new RoundButton("New", codex::CardBg, codex::HoverBg, codex::Text);
    new_btn->set_icon(IconId::Plus, 13.0f);
    head_row->dock(title, Dock::Left);
    head_row->dock(new_btn, Dock::Right);

    auto sessions = new ScrollView;
    side_dock->dock(sessions, Dock::Fill);
    auto session_stack = new StackPanel(Orientation::Vertical);
    session_stack->set_spacing(0.0f);
    sessions->set_content(session_stack);

    auto s1 = new SessionItem("Refactor measure cache", "Today 10:24");
    s1->set_selected(true);
    session_stack->append_child(s1);
    session_stack->append_child(new SessionItem("Grid star rows support", "Today 09:51"));
    session_stack->append_child(new SessionItem("Min/max size constraints", "Yesterday"));
    session_stack->append_child(new SessionItem("Fix layout pass pollution", "Yesterday"));
    session_stack->append_child(new SessionItem("Layout test cleanup", "2 days ago"));

    // ===== Main area =====
    auto main_pane = new Pane(codex::WindowBg);
    auto main_dock = new DockPanel;
    main_pane->set_content(main_dock);

    // Topbar: chat title + model name
    auto topbar = new Pane(codex::WindowBg, 44.0f);
    main_dock->dock(topbar, Dock::Top);
    auto top_row = new DockPanel;
    top_row->set_padding(14.0f);
    topbar->set_content(top_row);
    auto chat_title = new Label("Refactor measure cache");
    chat_title->set_bold(true);
    chat_title->set_align(TextAlignH::Left, TextAlignV::Center);
    auto model = new Label("gpt-5.2-codex");
    model->set_small(true);
    model->set_text_color(codex::TextSecondary);
    model->set_align(TextAlignH::Left, TextAlignV::Center);
    top_row->dock(chat_title, Dock::Left);
    top_row->dock(model, Dock::Right);

    auto composer = new Composer;
    main_dock->dock(composer, Dock::Bottom);

    auto messages = new ScrollView;
    main_dock->dock(messages, Dock::Fill);
    auto msg_stack = new StackPanel(Orientation::Vertical);
    msg_stack->set_spacing(10.0f);
    msg_stack->set_padding(12.0f);
    messages->set_content(msg_stack);

    msg_stack->append_child(new MessageCard(
        true, "Can you refactor the measure pipeline? I want measure() to cache the desired size "
              "so arrange can reuse it instead of re-measuring every frame."));
    msg_stack->append_child(new ToolCard("bash", "cmake --build build --config Release", "Completed"));
    msg_stack->append_child(new ToolCard(
        "edit", "src/widget.cpp\n  Widget::measure() dispatcher\n  + measure_impl() per control", "Completed"));
    msg_stack->append_child(new MessageCard(
        false, "Done. Widget::measure now caches into desired_size_, and every control moved its "
               "logic into measure_impl. Layout::perform_layout refreshes the cache before arrange, "
               "so nothing goes stale."));
    msg_stack->append_child(new MessageCard(
        true, "Nice. Now add min/max size constraints so widgets can clamp their own size."));
    msg_stack->append_child(new ToolCard("grep", "grep -rn \"measure_impl\" include/yuzuki src", "Completed"));
    msg_stack->append_child(new MessageCard(
        false, "min/max clamps now live in the measure dispatcher. Layout tests: 320 checks, "
               "0 failures."));

    // ===== Root =====
    auto root = new DockPanel;
    root->dock(sidebar, Dock::Left);
    root->dock(main_pane, Dock::Fill);
    return root;
}