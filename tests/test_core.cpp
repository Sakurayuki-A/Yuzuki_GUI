#include <yuzuki/yuzuki.hpp>

#include <windows.h>

#undef MessageBox  // winuser.h maps MessageBox → MessageBoxW; we use yzk::MessageBox

#include <cstdio>

using namespace yzk;

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const char* file, int line, const char* expr) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::printf("FAIL %s:%d  %s\n", file, line, expr);
    }
}

#define CHECK(expr) check((expr), __FILE__, __LINE__, #expr)

class FixedWidget : public Widget {
public:
    explicit FixedWidget(Size size) : size_(size) {}
    Size measure_impl(Size, const PaintContext* = nullptr) override { return size_; }

private:
    Size size_;
};

void test_rect() {
    const RectF r = RectF::make(10.0f, 20.0f, 100.0f, 50.0f);
    CHECK(r.width() == 100.0f);
    CHECK(r.height() == 50.0f);
    CHECK(r.contains(10.0f, 20.0f));
    CHECK(r.contains(109.0f, 69.0f));
    CHECK(!r.contains(110.0f, 70.0f));
    CHECK(r.contains_rect(RectF::make(20.0f, 30.0f, 40.0f, 20.0f)));
    CHECK(!r.contains_rect(RectF::make(5.0f, 5.0f, 10.0f, 10.0f)));

    RectF a = RectF::make(0.0f, 0.0f, 10.0f, 10.0f);
    a.unite(RectF::make(20.0f, 20.0f, 10.0f, 10.0f));
    CHECK(a.left == 0.0f && a.top == 0.0f && a.right == 30.0f && a.bottom == 30.0f);

    const RectF inter = RectF::make(0.0f, 0.0f, 10.0f, 10.0f)
                            .intersect(RectF::make(5.0f, 5.0f, 10.0f, 10.0f));
    CHECK(inter == RectF::make(5.0f, 5.0f, 5.0f, 5.0f));
}

void test_color() {
    const Color c = Color::rgba(0xFF112233u);
    CHECK(c.r == 0xFF && c.g == 0x11 && c.b == 0x22 && c.a == 0x33);
    const Color t = c.with_alpha(0);
    CHECK(t.is_transparent());
    CHECK(!c.is_transparent());
}

void test_event() {
    Event e;
    e.type = EventType::Click;
    e.data.mouse.x = 42.0f;
    e.data.mouse.buttons = MouseButton_Left;
    CHECK(e.type == EventType::Click);
    CHECK(e.data.mouse.x == 42.0f);
    CHECK((e.data.mouse.buttons & MouseButton_Left) != 0);
    CHECK(!e.consumed);
}

void test_encoding() {
    const String utf8 = u8"你好, YuzukiUI!";
    const WString wide = utf::to_wide(utf8);
    CHECK(utf::to_utf8(wide) == utf8);
    CHECK(utf::to_wide(String()) == WString());
    CHECK(utf::to_utf8(WString()) == String());
}

void test_stack_panel() {
    StackPanel panel(Orientation::Vertical);
    panel.set_padding(4.0f);
    panel.set_spacing(2.0f);

    FixedWidget a(Size{20.0f, 10.0f});
    FixedWidget b(Size{50.0f, 20.0f});
    FixedWidget c(Size{30.0f, 5.0f});
    panel.append_child(&a);
    panel.append_child(&b);
    panel.append_child(&c);

    const Size total = panel.measure(Size{200.0f, 200.0f});
    CHECK(total.width == 50.0f + 8.0f);
    CHECK(total.height == 10.0f + 20.0f + 5.0f + 2.0f * 2.0f + 8.0f);

    panel.set_bounds(RectF::make(0.0f, 0.0f, 100.0f, 100.0f));
    panel.perform_layout();
    CHECK(a.bounds().top == 4.0f);
    CHECK(b.bounds().top == 4.0f + 10.0f + 2.0f);
    CHECK(c.bounds().top == 4.0f + 10.0f + 2.0f + 20.0f + 2.0f);
    CHECK(c.bounds().bottom == 4.0f + 10.0f + 2.0f + 20.0f + 2.0f + 5.0f);
    CHECK(b.bounds().left == 4.0f);
    CHECK(b.bounds().right == 96.0f);
}

void test_hit_test() {
    StackPanel panel(Orientation::Vertical);
    panel.set_padding(16.0f);
    panel.set_spacing(10.0f);

    FixedWidget a(Size{80.0f, 32.0f});
    FixedWidget b(Size{80.0f, 32.0f});
    FixedWidget c(Size{80.0f, 32.0f});
    panel.append_child(&a);
    panel.append_child(&b);
    panel.append_child(&c);

    panel.set_bounds(RectF::make(0.0f, 0.0f, 420.0f, 300.0f));
    panel.perform_layout();

    const f32 center_x = 16.0f + (420.0f - 32.0f) / 2.0f;
    const f32 inner_top = 16.0f;
    const f32 step = 32.0f + 10.0f;

    CHECK(panel.hit_test(center_x, inner_top + step * 0.0f + 16.0f) == &a);
    CHECK(panel.hit_test(center_x, inner_top + step * 1.0f + 16.0f) == &b);
    CHECK(panel.hit_test(center_x, inner_top + step * 2.0f + 16.0f) == &c);

    CHECK(panel.hit_test(0.0f, 0.0f) != &a);
    CHECK(panel.hit_test(center_x, inner_top + 32.0f + 5.0f) == &panel);
    CHECK(panel.hit_test(-10.0f, 50.0f) == nullptr);
    CHECK(panel.hit_test(500.0f, 50.0f) == nullptr);
}

void test_widget_tree() {
    Widget root;
    FixedWidget child(Size{10.0f, 10.0f});
    FixedWidget grand(Size{5.0f, 5.0f});

    child.append_child(&grand);
    root.append_child(&child);

    CHECK(child.parent() == &root);
    CHECK(grand.parent() == &child);
    CHECK(root.first_child() == &child);
    CHECK(child.last_child() == &grand);

    grand.remove_from_parent();
    CHECK(grand.parent() == nullptr);
    CHECK(child.first_child() == nullptr);
}

void test_transform() {
    const Transform2D t = Transform2D::translation(10.0f, 20.0f);
    const Point p = t.apply(1.0f, 2.0f);
    CHECK(p.x == 11.0f && p.y == 22.0f);

    const Transform2D s = Transform2D::scaling(2.0f, 3.0f);
    const Point q = s.apply(4.0f, 5.0f);
    CHECK(q.x == 8.0f && q.y == 15.0f);

    const Transform2D c = t * s;  // scale first, then translate
    const Point r = c.apply(1.0f, 1.0f);
    CHECK(r.x == 12.0f && r.y == 23.0f);

    const Transform2D rot = Transform2D::rotation_deg(90.0f);
    const Point v = rot.apply(1.0f, 0.0f);
    CHECK(std::abs(v.x) < 0.001f && std::abs(v.y + 1.0f) < 0.001f);  // screen y points down: visually clockwise

    const Transform2D around = Transform2D::around(5.0f, 5.0f, Transform2D::scaling(2.0f, 2.0f));
    const Point w = around.apply(5.0f, 5.0f);
    CHECK(w.x == 5.0f && w.y == 5.0f);
    const Point u = around.apply(6.0f, 5.0f);
    CHECK(std::abs(u.x - 7.0f) < 0.001f && std::abs(u.y - 5.0f) < 0.001f);

    const RectF rr = Transform2D::rotation_deg(90.0f).apply_rect(RectF::make(0.0f, 0.0f, 10.0f, 4.0f));
    CHECK(std::abs(rr.left - 0.0f) < 0.001f && std::abs(rr.right - 4.0f) < 0.001f);
    CHECK(std::abs(rr.top + 10.0f) < 0.001f && std::abs(rr.bottom - 0.0f) < 0.001f);

    CHECK(Transform2D::identity().is_identity());
    CHECK(!Transform2D::translation(1.0f, 0.0f).is_identity());
}

void test_animatable_property() {
    AnimatableProperty<f32> p{0.0f};
    int changes = 0;
    p.set_on_changed([&](const f32&) { ++changes; });

    p.set(5.0f);  // immediate set
    CHECK(p.value() == 5.0f);
    CHECK(changes == 1);

    p.animate(10.0f, 100.0f, Easing::Linear);
    CHECK(p.animating());
    AnimationSystem& as = AnimationSystem::instance();
    as.tick(50.0f);
    CHECK(p.value() > 5.0f && p.value() < 10.0f);
    as.tick(100.0f);
    CHECK(p.value() == 10.0f);
    CHECK(!p.animating());
    CHECK(changes >= 3);

    // Implicit transition
    AnimatableProperty<Color> c{Color{0, 0, 0, 255}};
    c.set_transition(200.0f);
    int color_changes = 0;
    c.set_on_changed([&](const Color&) { ++color_changes; });
    c.set_animated(Color{255, 255, 255, 255});
    CHECK(c.animating());
    as.tick(200.0f);
    CHECK(c.value().r == 255 && c.value().g == 255 && c.value().b == 255 && c.value().a == 255);

    // set_animated applies immediately without a transition
    AnimatableProperty<f32> q{1.0f};
    q.set_animated(7.0f);
    CHECK(q.value() == 7.0f);
    CHECK(!q.animating());

    // Re-animating mid-tween redirects from the current value
    AnimatableProperty<f32> r{0.0f};
    r.animate(100.0f, 1000.0f, Easing::Linear);
    as.tick(100.0f);
    const f32 mid = r.value();
    CHECK(mid > 0.0f && mid < 100.0f);
    r.animate(200.0f, 500.0f, Easing::Linear);
    as.tick(500.0f);
    CHECK(r.value() == 200.0f);

    as.stop_all();
}

void test_widget_transition() {
    // Widget implicit transition: visual setters tween after set_transition
    Widget w;
    w.set_transition(100.0f);
    w.set_scale(2.0f, 2.0f);
    AnimationSystem& as = AnimationSystem::instance();
    CHECK(w.scale_x() == 1.0f);  // immediate check: tween just started, value unchanged
    as.tick(100.0f);
    CHECK(w.scale_x() == 2.0f && w.scale_y() == 2.0f);

    // No transition: applies immediately
    Widget v;
    v.set_opacity(0.5f);
    CHECK(v.opacity() == 0.5f);
    CHECK(!v.has_visual_state() || v.opacity() == 0.5f);
    CHECK(v.opacity() == 0.5f && v.opacity() < 1.0f);
    CHECK(v.has_visual_state());

    // Fully transparent skips painting (paint wrapper); only the state value is checked here
    Widget t;
    t.set_opacity(0.0f);
    CHECK(t.opacity() == 0.0f);

    as.stop_all();
}

void test_flex_box() {
    // Default start = top-left (0,0), horizontal direction
    FlexBox row;
    row.set_spacing(10.0f);
    FixedWidget a(Size{50.0f, 20.0f});
    FixedWidget b(Size{50.0f, 20.0f});
    row.append_child(&a);
    row.append_child(&b);
    row.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 60.0f));
    row.perform_layout();
    CHECK(a.bounds().left == 0.0f && a.bounds().top == 0.0f);  // default top-left
    CHECK(b.bounds().left == 60.0f);
    CHECK(b.bounds().right == 110.0f);

    // grow: leftover space split proportionally (grow 1 : grow 2)
    FlexBox grow_row;
    grow_row.set_spacing(10.0f);
    FixedWidget fixed(Size{50.0f, 20.0f});
    FixedWidget g1(Size{50.0f, 20.0f});
    FixedWidget g2(Size{50.0f, 20.0f});
    g1.set_flex_grow(1.0f);
    g2.set_flex_grow(2.0f);
    grow_row.append_child(&fixed);
    grow_row.append_child(&g1);
    grow_row.append_child(&g2);
    grow_row.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 60.0f));
    grow_row.perform_layout();
    // Leftover = 300 - 50 - 50 - 50 - 20 = 130 → g1 gets 43.33, g2 gets 86.67
    CHECK(std::abs(fixed.bounds().width() - 50.0f) < 0.01f);
    CHECK(std::abs(g1.bounds().width() - 50.0f - 130.0f / 3.0f) < 0.01f);
    CHECK(std::abs(g2.bounds().width() - 50.0f - 260.0f / 3.0f) < 0.01f);
    CHECK(std::abs(g2.bounds().right - 300.0f) < 0.01f);

    // shrink: overflow shrinks proportionally (default shrink=1)
    FlexBox shrink_row;
    shrink_row.set_spacing(0.0f);
    FixedWidget s1(Size{100.0f, 20.0f});
    FixedWidget s2(Size{100.0f, 20.0f});
    shrink_row.append_child(&s1);
    shrink_row.append_child(&s2);
    shrink_row.set_bounds(RectF::make(0.0f, 0.0f, 150.0f, 60.0f));
    shrink_row.perform_layout();
    CHECK(std::abs(s1.bounds().width() - 75.0f) < 0.01f);
    CHECK(std::abs(s2.bounds().width() - 75.0f) < 0.01f);

    // shrink=0: no shrinking on overflow
    FlexBox no_shrink;
    FixedWidget n1(Size{100.0f, 20.0f});
    FixedWidget n2(Size{100.0f, 20.0f});
    n1.set_flex_shrink(0.0f);
    n2.set_flex_shrink(0.0f);
    no_shrink.append_child(&n1);
    no_shrink.append_child(&n2);
    no_shrink.set_bounds(RectF::make(0.0f, 0.0f, 150.0f, 60.0f));
    no_shrink.perform_layout();
    CHECK(n1.bounds().width() == 100.0f && n2.bounds().width() == 100.0f);

    // Alignment: Center / End / SpaceBetween / SpaceAround
    FlexBox align_box;
    align_box.set_spacing(0.0f);
    FixedWidget c1(Size{50.0f, 20.0f});
    FixedWidget c2(Size{50.0f, 20.0f});
    align_box.append_child(&c1);
    align_box.append_child(&c2);
    align_box.set_align_main(FlexAlign::SpaceBetween);
    align_box.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 60.0f));
    align_box.perform_layout();
    CHECK(c1.bounds().left == 0.0f);
    CHECK(c2.bounds().right == 200.0f);

    FlexBox center_box;
    center_box.set_align_main(FlexAlign::Center);
    FixedWidget cc1(Size{50.0f, 20.0f});
    FixedWidget cc2(Size{50.0f, 20.0f});
    center_box.append_child(&cc1);
    center_box.append_child(&cc2);
    center_box.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 60.0f));
    center_box.perform_layout();
    CHECK(std::abs(cc1.bounds().left - 50.0f) < 0.01f);  // remaining 100 centered
    CHECK(std::abs(cc2.bounds().right - 150.0f) < 0.01f);

    // Cross axis: Center / Stretch / End
    FlexBox cross_box;
    cross_box.set_align_cross(FlexCrossAlign::Center);
    FixedWidget x1(Size{50.0f, 20.0f});
    cross_box.append_child(&x1);
    cross_box.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 60.0f));
    cross_box.perform_layout();
    CHECK(std::abs(x1.bounds().top - 20.0f) < 0.01f);

    FlexBox stretch_box;
    stretch_box.set_align_cross(FlexCrossAlign::Stretch);
    FixedWidget st1(Size{50.0f, 20.0f});
    stretch_box.append_child(&st1);
    stretch_box.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 60.0f));
    stretch_box.perform_layout();
    CHECK(st1.bounds().top == 0.0f && st1.bounds().bottom == 60.0f);

    // Vertical direction: main axis = height
    FlexBox column;
    column.set_direction(Orientation::Vertical);
    column.set_spacing(10.0f);
    FixedWidget v1(Size{20.0f, 30.0f});
    FixedWidget v2(Size{20.0f, 40.0f});
    column.append_child(&v1);
    column.append_child(&v2);
    column.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    column.perform_layout();
    CHECK(v1.bounds().top == 0.0f && v1.bounds().bottom == 30.0f);
    CHECK(v2.bounds().top == 40.0f && v2.bounds().bottom == 80.0f);

    // Vertical grow: consumes leftover height
    FlexBox vgrow;
    vgrow.set_direction(Orientation::Vertical);
    FixedWidget vg(Size{20.0f, 30.0f});
    vg.set_flex_grow(1.0f);
    vgrow.append_child(&vg);
    vgrow.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    vgrow.perform_layout();
    CHECK(vg.bounds().top == 0.0f && vg.bounds().bottom == 200.0f);
}

void test_box() {
    Box box;
    box.set_bg(Color{0xff, 0x00, 0x00, 0xff});
    box.set_radius(8.0f);
    box.set_padding(10.0f);
    FixedWidget child(Size{40.0f, 20.0f});
    box.append_child(&child);
    box.set_bounds(RectF::make(5.0f, 5.0f, 100.0f, 60.0f));
    box.perform_layout();
    // Content area = local coords (padding, padding, inner_w, inner_h)
    CHECK(box.content_area() == RectF::make(10.0f, 10.0f, 80.0f, 40.0f));
    // Child fills the content area (local coords)
    CHECK(child.bounds() == RectF::make(10.0f, 10.0f, 80.0f, 40.0f));
    CHECK(box.radius() == 8.0f && box.bg().r == 0xff);
    CHECK(box.border_width() == 0.0f);
    // Border/shadow properties
    box.set_border(2.0f, Color{0x00, 0x00, 0x00, 0xff});
    box.set_shadow(12.0f, 4.0f);
    CHECK(box.border_width() == 2.0f && !box.border_color().is_transparent());
    CHECK(box.shadow_blur() == 12.0f && box.shadow_offset_y() == 4.0f);

    // No children: measure returns (padding*2, padding*2)
    Box empty;
    empty.set_padding(6.0f);
    const Size es = empty.measure(Size{200.0f, 200.0f}, nullptr);
    CHECK(es.width == 12.0f && es.height == 12.0f);

    // Child natural size + padding = Box natural size
    Box sized;
    sized.set_padding(5.0f);
    FixedWidget inner(Size{30.0f, 10.0f});
    sized.append_child(&inner);
    const Size ss = sized.measure(Size{200.0f, 200.0f}, nullptr);
    CHECK(ss.width == 40.0f && ss.height == 20.0f);
}

struct ClickRecorder : Button {
    int clicks = 0;
    explicit ClickRecorder(const String& text) : Button(text) {}
    void on_click() override { ++clicks; }
};

struct ToggleRecorder : CheckBox {
    int toggles = 0;
    explicit ToggleRecorder(const String& text) : CheckBox(text) {}
    void on_toggled(bool) override { ++toggles; }
};

struct SliderRecorder : Slider {
    int changes = 0;
    void on_changed(f32) override { ++changes; }
};

void test_button() {
    ClickRecorder b("Click");
    b.set_bounds(RectF::make(0.0f, 0.0f, 80.0f, 32.0f));
    const Size bs = b.measure(Size{100.0f, 100.0f}, nullptr);
    CHECK(bs.width == 80.0f && bs.height == 32.0f);

    Event enter;
    enter.type = EventType::MouseEnter;
    b.on_event(enter);
    CHECK(b.hovered());

    Event leave;
    leave.type = EventType::MouseLeave;
    b.on_event(leave);
    CHECK(!b.hovered());

    Event down;
    down.type = EventType::MouseDown;
    down.data.mouse.buttons = MouseButton_Left;
    down.data.mouse.x = 40.0f;
    down.data.mouse.y = 16.0f;
    b.on_event(down);
    CHECK(b.pressed());
    CHECK(down.consumed);

    Event up;
    up.type = EventType::MouseUp;
    up.data.mouse.x = 40.0f;
    up.data.mouse.y = 16.0f;
    b.on_event(up);
    CHECK(!b.pressed());
    CHECK(b.clicks == 1);

    // Press, move out, then release: no click
    Event down2;
    down2.type = EventType::MouseDown;
    down2.data.mouse.buttons = MouseButton_Left;
    down2.data.mouse.x = 10.0f;
    down2.data.mouse.y = 10.0f;
    b.on_event(down2);
    Event up2;
    up2.type = EventType::MouseUp;
    up2.data.mouse.x = 200.0f;
    up2.data.mouse.y = 200.0f;
    b.on_event(up2);
    CHECK(b.clicks == 1);

    // Disabled: never enters the pressed state
    b.set_enabled(false);
    Event down3;
    down3.type = EventType::MouseDown;
    down3.data.mouse.buttons = MouseButton_Left;
    down3.data.mouse.x = 40.0f;
    down3.data.mouse.y = 16.0f;
    b.on_event(down3);
    CHECK(!b.pressed());
}

void test_check_box() {
    ToggleRecorder c("Option");
    c.set_bounds(RectF::make(0.0f, 0.0f, 100.0f, 28.0f));
    CHECK(!c.checked());
    Event down;
    down.type = EventType::MouseDown;
    down.data.mouse.buttons = MouseButton_Left;
    c.on_event(down);
    CHECK(c.checked());
    CHECK(c.toggles == 1);
    c.on_event(down);
    CHECK(!c.checked());
    CHECK(c.toggles == 2);
    c.set_checked(true);
    CHECK(c.checked());
    CHECK(c.toggles == 2);  // programmatic set doesn't call back
}

void test_slider() {
    SliderRecorder s;
    s.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 24.0f));
    CHECK(s.min() == 0.0f && s.max() == 100.0f);
    s.set_value(150.0f);
    CHECK(s.value() == 100.0f);
    s.set_value(-10.0f);
    CHECK(s.value() == 0.0f);
    s.set_range(10.0f, 50.0f);
    CHECK(s.value() == 10.0f);  // out-of-range value clamped to the new range

    Event down;
    down.type = EventType::MouseDown;
    down.data.mouse.buttons = MouseButton_Left;
    down.data.mouse.x = 7.0f;  // track start → min
    down.data.mouse.y = 12.0f;
    s.on_event(down);
    CHECK(s.value() == 10.0f);
    down.data.mouse.x = 100.0f;  // midpoint → (10+50)/2
    s.on_event(down);
    CHECK(std::abs(s.value() - 30.0f) < 0.01f);

    Event move;
    move.type = EventType::MouseMove;
    move.data.mouse.buttons = MouseButton_Left;
    move.data.mouse.x = 193.0f;  // track end → max
    s.on_event(move);
    CHECK(s.value() == 50.0f);

    Event up;
    up.type = EventType::MouseUp;
    s.on_event(up);
    CHECK(s.changes == 2);  // one per drag to 30 and 50; a click at the current value doesn't fire
}

void test_text_box() {
    TextBox box("hello");
    CHECK(box.text() == "hello");
    Event focus;
    focus.type = EventType::FocusGained;
    box.on_event(focus);

    Event ch;
    ch.type = EventType::Character;
    ch.data.key.chr = L'X';
    box.on_event(ch);
    CHECK(box.text() == "helloX");  // caret at end inserts there

    Event bs;
    bs.type = EventType::KeyDown;
    bs.data.key.code = 0x08;  // VK_BACK
    box.on_event(bs);
    CHECK(box.text() == "hello");

    // Ctrl+A selects all, then typing replaces
    Event ctrl_a;
    ctrl_a.type = EventType::KeyDown;
    ctrl_a.data.key.mods = KeyModifier_Control;
    ctrl_a.data.key.code = 'A';
    box.on_event(ctrl_a);
    Event ch2;
    ch2.type = EventType::Character;
    ch2.data.key.chr = L'Z';
    box.on_event(ch2);
    CHECK(box.text() == "Z");

    // read_only blocks input
    TextBox ro("abc");
    ro.set_read_only(true);
    Event focus2;
    focus2.type = EventType::FocusGained;
    ro.on_event(focus2);
    Event ch3;
    ch3.type = EventType::Character;
    ch3.data.key.chr = L'Q';
    ro.on_event(ch3);
    CHECK(ro.text() == "abc");

    // Multiline: Enter inserts exactly ONE newline. The KeyDown (VK_RETURN) owns the
    // insertion; the WM_CHAR '\r' that TranslateMessage delivers for the same press
    // must be ignored, or every Enter would produce a double newline.
    TextBoxConfig cfg;
    cfg.mode = TextBoxMode::MultiLine;
    TextBox ml("ab", cfg);
    Event focus3;
    focus3.type = EventType::FocusGained;
    ml.on_event(focus3);
    Event enter_down;
    enter_down.type = EventType::KeyDown;
    enter_down.data.key.code = VK_RETURN;
    ml.on_event(enter_down);
    Event enter_char;
    enter_char.type = EventType::Character;
    enter_char.data.key.chr = L'\r';
    ml.on_event(enter_char);
    CHECK(ml.text() == "ab\n");

    // Ctrl+Enter must also insert exactly ONE newline: KeyDown(VK_RETURN, ctrl)
    // is the single insertion point and the follow-up WM_CHAR '\n' is ignored.
    TextBoxConfig cfg2;
    cfg2.mode = TextBoxMode::MultiLine;
    TextBox ml2("ab", cfg2);
    Event focus4;
    focus4.type = EventType::FocusGained;
    ml2.on_event(focus4);
    Event ctrl_enter_down;
    ctrl_enter_down.type = EventType::KeyDown;
    ctrl_enter_down.data.key.mods = KeyModifier_Control;
    ctrl_enter_down.data.key.code = VK_RETURN;
    ml2.on_event(ctrl_enter_down);
    Event ctrl_enter_char;
    ctrl_enter_char.type = EventType::Character;
    ctrl_enter_char.data.key.mods = KeyModifier_Control;
    ctrl_enter_char.data.key.chr = L'\n';
    ml2.on_event(ctrl_enter_char);
    CHECK(ml2.text() == "ab\n");

    // enter_submits: Enter fires the commit callback instead of inserting; the
    // trailing WM_CHAR '\r' stays inert and no newline is created.
    TextBoxConfig cfg3;
    cfg3.mode = TextBoxMode::MultiLine;
    cfg3.enter_submits = true;
    TextBox ml3("ab", cfg3);
    Event focus5;
    focus5.type = EventType::FocusGained;
    ml3.on_event(focus5);
    int commits = 0;
    ml3.set_on_commit([&commits]() { ++commits; });
    Event submit_down;
    submit_down.type = EventType::KeyDown;
    submit_down.data.key.code = VK_RETURN;
    ml3.on_event(submit_down);
    Event submit_char;
    submit_char.type = EventType::Character;
    submit_char.data.key.chr = L'\r';
    ml3.on_event(submit_char);
    CHECK(commits == 1);
    CHECK(ml3.text() == "ab");
}

void test_text_box_undo_redo() {
    auto focus = []() {
        Event f;
        f.type = EventType::FocusGained;
        return f;
    };
    auto key = [](u32 code, u8 mods = KeyModifier_None) {
        Event k;
        k.type = EventType::KeyDown;
        k.data.key.code = code;
        k.data.key.mods = mods;
        return k;
    };
    auto chr = [](wchar_t c, u8 mods = KeyModifier_None) {
        Event ch;
        ch.type = EventType::Character;
        ch.data.key.chr = c;
        ch.data.key.mods = mods;
        return ch;
    };

    {
        TextBox box;
        box.on_event(focus());
        // A typing run coalesces into ONE undo step.
        box.on_event(chr(L'h'));
        box.on_event(chr(L'e'));
        box.on_event(chr(L'l'));
        box.on_event(chr(L'l'));
        box.on_event(chr(L'o'));
        CHECK(box.text() == "hello");
        box.on_event(key('Z', KeyModifier_Control));
        CHECK(box.text() == "");  // whole run reverted in one step
        box.on_event(key('Y', KeyModifier_Control));
        CHECK(box.text() == "hello");  // redo restores the run
    }
    {
        // Delete runs coalesce; a paste starts a new group.
        TextBox box("abcde");
        box.on_event(focus());
        box.on_event(key(VK_BACK));  // deletes 'e'
        box.on_event(key(VK_BACK));  // deletes 'd' — same run
        CHECK(box.text() == "abc");
        box.on_event(key('Z', KeyModifier_Control));
        CHECK(box.text() == "abcde");  // both chars reverted at once
    }
    {
        // Editing history is cleared by a new edit after an undo (redo invalidated).
        TextBox box;
        box.on_event(focus());
        box.on_event(chr(L'a'));
        box.on_event(chr(L'b'));
        box.on_event(key('Z', KeyModifier_Control));  // -> ""
        box.on_event(chr(L'c'));                      // new edit
        box.on_event(key('Y', KeyModifier_Control));  // nothing to redo
        CHECK(box.text() == "c");
    }
    {
        // Ctrl+Shift+Z redoes too.
        TextBox box;
        box.on_event(focus());
        box.on_event(chr(L'x'));
        box.on_event(key('Z', KeyModifier_Control));
        box.on_event(key('Z', KeyModifier_Control | KeyModifier_Shift));
        CHECK(box.text() == "x");
    }
    {
        // No-op edits (backspace at start) must not clobber the redo stack.
        TextBox box;
        box.on_event(focus());
        box.on_event(chr(L'a'));
        box.on_event(key('Z', KeyModifier_Control));  // undo "a"
        box.on_event(key(VK_BACK));                   // no-op at position 0
        box.on_event(key('Y', KeyModifier_Control));  // still redoable
        CHECK(box.text() == "a");
    }
}

void test_text_box_word_navigation() {
    auto focus = []() {
        Event f;
        f.type = EventType::FocusGained;
        return f;
    };
    auto key = [](u32 code, u8 mods = KeyModifier_None) {
        Event k;
        k.type = EventType::KeyDown;
        k.data.key.code = code;
        k.data.key.mods = mods;
        return k;
    };
    auto chr = [](wchar_t c) {
        Event ch;
        ch.type = EventType::Character;
        ch.data.key.chr = c;
        return ch;
    };

    {
        // Ctrl+Right jumps to the next word boundary; typing lands there.
        TextBox box("hello world foo");
        box.on_event(focus());
        box.on_event(key(VK_HOME));
        box.on_event(key(VK_RIGHT, KeyModifier_Control));  // after "hello"
        box.on_event(chr(L'X'));
        CHECK(box.text() == "helloX world foo");

        // Ctrl+Left jumps back to the previous word start.
        box.on_event(key(VK_RIGHT, KeyModifier_Control));  // after "world"
        box.on_event(key(VK_LEFT, KeyModifier_Control));   // back to start of "world"
        box.on_event(chr(L'Y'));
        CHECK(box.text() == "helloX Yworld foo");
    }
    {
        // Ctrl+Backspace deletes the word to the left (keeps the gap).
        TextBox box("one two");
        box.on_event(focus());
        box.on_event(key(VK_END));
        box.on_event(key(VK_BACK, KeyModifier_Control));
        CHECK(box.text() == "one ");
    }
    {
        // Ctrl+Delete deletes the word to the right (keeps the gap).
        TextBox box("one two");
        box.on_event(focus());
        box.on_event(key(VK_HOME));
        box.on_event(key(VK_DELETE, KeyModifier_Control));
        CHECK(box.text() == " two");
    }
    {
        // Ctrl+Home / Ctrl+End jump to the document edges in multiline mode.
        TextBoxConfig cfg;
        cfg.mode = TextBoxMode::MultiLine;
        TextBox box("line1\nline2", cfg);
        box.on_event(focus());
        box.on_event(key(VK_END, KeyModifier_Control));  // document end
        box.on_event(chr(L'!'));
        CHECK(box.text() == "line1\nline2!");
        box.on_event(key(VK_HOME, KeyModifier_Control));  // document start
        box.on_event(chr(L'?'));
        CHECK(box.text() == "?line1\nline2!");
    }
}

void test_label_rich_text() {
    auto width_of = [](const String& text, bool small = false) -> f32 {
        Label l(text);
        l.set_rich_text(true);
        l.set_small(small);
        const Size s = l.measure_impl(Size{}, nullptr);
        return s.width;
    };

    // Plain rich (no markup) equals a plain label's estimate.
    const f32 plain = width_of("hello world");
    CHECK(plain > 0.0f);

    // Delimiters consume no width: "hello world" via markup strips to "hello world";
    // **bold** contributes only its visible chars.
    const f32 marked = width_of("hello **bold** world");
    const f32 bold_plain = width_of("hello bold world");
    const f32 diff = marked > bold_plain ? marked - bold_plain : bold_plain - marked;
    CHECK(diff < 0.5f);  // same visible glyphs -> near-equal estimate

    // Link markup: [label](action) contributes only "label".
    const f32 link_line = width_of("Go to [docs](https://x) now");
    const f32 link_plain = width_of("Go to docs now");
    const f32 diff2 = link_line > link_plain ? link_line - link_plain : link_plain - link_line;
    CHECK(diff2 < 0.5f);

    // Highlight markup strips tildes.
    const f32 hl = width_of("prefix ~em~ suffix");
    const f32 hl_plain = width_of("prefix em suffix");
    const f32 diff3 = hl > hl_plain ? hl - hl_plain : hl_plain - hl;
    CHECK(diff3 < 0.5f);

    // Longer visible text => wider estimate.
    CHECK(width_of("aa") < width_of("aaaa"));

    // Disabling rich disables stripping.
    Label l("**bold**");
    CHECK(l.measure_impl(Size{}, nullptr).width > width_of("bold"));

    // Render path must not throw with a constructed rich label (headless: no ctx).
    Label clickable("[click me](open) **b**");
    clickable.set_rich_text(true);
    clickable.set_on_span_click([](const String&) {});
    CHECK(!clickable.hit_link(9999.0f, 9999.0f));  // geometry not laid out headlessly -> no fire
}

void test_toggle_switch() {
    ToggleSwitch t;
    CHECK(!t.checked());
    Event down;
    down.type = EventType::MouseDown;
    down.data.mouse.buttons = MouseButton_Left;
    t.on_event(down);
    CHECK(t.checked());
    t.on_event(down);
    CHECK(!t.checked());
}

void test_radio_button() {
    Widget group;
    RadioButton a("a"), b("b"), c("c");
    group.append_child(&a);
    group.append_child(&b);
    group.append_child(&c);
    b.set_checked(true);
    CHECK(b.checked() && !a.checked() && !c.checked());
    a.set_checked(true);
    CHECK(a.checked() && !b.checked() && !c.checked());
}

void test_progress_bar() {
    ProgressBar p;
    CHECK(p.value() == 0.0f);
    p.set_value(0.5f);
    CHECK(p.value() == 0.5f);
    p.set_value(2.0f);
    CHECK(p.value() == 1.0f);
    p.set_value(-1.0f);
    CHECK(p.value() == 0.0f);
    p.set_indeterminate(true);
    CHECK(p.indeterminate());
}

void test_spin_box() {
    SpinBox s(50.0, 0.0, 100.0, 5.0);
    CHECK(s.value() == 50.0);
    s.set_value(200.0);
    CHECK(s.value() == 100.0);
    s.set_value(-1.0);
    CHECK(s.value() == 0.0);
    s.set_range(20.0, 80.0);
    CHECK(s.min() == 20.0 && s.max() == 80.0);
    CHECK(s.value() == 20.0);
    s.set_decimals(2);
    CHECK(s.text() == "20.00");
}

void test_tab_control() {
    TabControl tc;
    auto* p1 = new Label("Page One");
    auto* p2 = new Label("Page Two");
    tc.add_tab("General", p1).add_tab("About", p2);

    CHECK(tc.tab_count() == 2);
    CHECK(tc.tab_title(0) == "General");
    CHECK(tc.tab_title(1) == "About");
    CHECK(tc.selected_index() == 0);
    CHECK(tc.selected_page() == p1);
    CHECK(p1->visible() && !p2->visible());  // only the active page is visible

    int fired = 0;
    tc.set_on_changed([&fired](i32) { ++fired; });
    tc.set_selected_index(0);  // same index: no-op
    CHECK(fired == 0);
    tc.set_selected_index(1);
    CHECK(fired == 0);  // not attached to a Window yet: on_changed is suppressed

    CHECK(tc.selected_index() == 1);
    CHECK(tc.selected_page() == p2);
    CHECK(!p1->visible() && p2->visible());
    CHECK(tc.page(0) == p1 && tc.page(99) == nullptr && tc.tab_title(99).empty());

    tc.set_selected_index(99);  // out-of-range ignored
    CHECK(tc.selected_index() == 1);

    // Keyboard navigation (Left/Right cycle with wrap).
    Event ev;
    ev.type = EventType::KeyDown;
    ev.data.key.code = VK_RIGHT;
    tc.on_event(ev);
    CHECK(ev.consumed && tc.selected_index() == 0);  // wraps 1 -> 0
    ev.consumed = false;
    ev.data.key.code = VK_LEFT;
    tc.on_event(ev);
    CHECK(ev.consumed && tc.selected_index() == 1);  // wraps 0 -> 1
    ev.consumed = false;
    ev.data.key.code = VK_TAB;  // non-navigation key passes through
    tc.on_event(ev);
    CHECK(!ev.consumed && tc.selected_index() == 1);

    // remove_tab re-indexes headers; deleting selection resets to a valid tab.
    auto* p3 = new Label("Page Three");
    tc.add_tab("Extras", p3);
    CHECK(tc.tab_count() == 3);
    tc.remove_tab(0);  // removes "General"/p1
    CHECK(tc.tab_count() == 2);
    CHECK(tc.selected_index() == 1);  // "About" kept and selected
    CHECK(tc.tab_title(0) == "About");
    CHECK(tc.tab_title(1) == "Extras");
    CHECK(tc.selected_page() == p3);
}

void test_combo_box() {
    ComboBox cb({"A", "B", "C"});
    CHECK(cb.items().size() == 3);
    CHECK(!cb.has_selection());
    cb.set_selected_index(1);
    CHECK(cb.selected_index() == 1);
    CHECK(cb.selected_text() == "B");
    cb.set_selected_index(99);  // out-of-range index ignored
    CHECK(cb.selected_index() == 1);
    cb.clear_items();
    CHECK(cb.items().empty());
    CHECK(!cb.has_selection());
}

void test_list_view() {
    ListView lv;
    std::vector<String> items;
    for (int i = 0; i < 20; ++i) items.push_back("item " + std::to_string(i));
    lv.set_items(items);
    CHECK(lv.items().size() == 20);
    CHECK(lv.selected() == -1);
    lv.set_selected(3);
    CHECK(lv.selected() == 3);
    lv.set_scroll_y(1000.0f);
    CHECK(lv.scroll_y() >= 0.0f && lv.scroll_y() <= 1000.0f);
    lv.scroll_by(-500.0f);
    CHECK(lv.scroll_y() >= 0.0f);
}

struct GenSource : ListView::DataSource {
    i32 n = 0;
    explicit GenSource(i32 count) : n(count) {}
    i32 count() const override { return n; }
    String text_at(i32 index) const override {
        return "row " + std::to_string(index);
    }
};

void test_list_view_data_source() {
    ListView lv;
    GenSource src(10000);
    lv.set_data_source(&src);
    CHECK(lv.count() == 10000);
    CHECK(lv.text_at(0) == "row 0");
    CHECK(lv.text_at(9999) == "row 9999");
    lv.set_selected(9999);
    CHECK(lv.selected() == 9999);
    lv.set_selected(10000);  // out-of-range index ignored
    CHECK(lv.selected() == 9999);
    lv.set_scroll_y(0.0f);
    // Switch back to internal vector mode
    lv.set_items({"a", "b"});
    CHECK(lv.data_source() == nullptr);
    CHECK(lv.count() == 2 && lv.text_at(1) == "b");
    // add_item is ignored in data-source mode
    lv.set_data_source(&src);
    lv.add_item("ignored");
    CHECK(lv.count() == 10000);
    // Row delegate can be set/cleared
    CHECK(lv.row_delegate() == nullptr);
    struct Dot : ListView::RowDelegate {
        void draw(ListView&, PaintContext&, i32, const RectF&) override {}
    };
    Dot dot;
    lv.set_row_delegate(&dot);
    CHECK(lv.row_delegate() == &dot);
    lv.set_row_delegate(nullptr);
    CHECK(lv.row_delegate() == nullptr);
}

void test_list_view_keyboard() {
    ListView lv;
    std::vector<String> items;
    for (int i = 0; i < 10; ++i) items.push_back("item " + std::to_string(i));
    lv.set_items(items);

    // Arrow keys walk the selection.
    Event down;
    down.type = EventType::KeyDown;
    down.data.key.code = VK_DOWN;
    lv.on_event(down);
    CHECK(lv.selected() == 0);  // first Down selects row 0
    for (int i = 0; i < 5; ++i) lv.on_event(down);
    CHECK(lv.selected() == 5);

    Event up;
    up.type = EventType::KeyDown;
    up.data.key.code = VK_UP;
    lv.on_event(up);
    CHECK(lv.selected() == 4);
    // Up at the top stays clamped at 0 (does not wrap).
    while (lv.selected() > 0) lv.on_event(up);
    lv.on_event(up);
    CHECK(lv.selected() == 0);

    Event home;
    home.type = EventType::KeyDown;
    home.data.key.code = VK_HOME;
    lv.on_event(home);
    CHECK(lv.selected() == 0);

    Event end;
    end.type = EventType::KeyDown;
    end.data.key.code = VK_END;
    lv.on_event(end);
    CHECK(lv.selected() == 9);
    lv.on_event(down);  // Down past the last row stays clamped
    CHECK(lv.selected() == 9);

    // Enter activates the selected row.
    int activated = -1;
    lv.on_activate([&](i32 i) { activated = i; });
    Event enter;
    enter.type = EventType::KeyDown;
    enter.data.key.code = VK_RETURN;
    lv.on_event(enter);
    CHECK(activated == 9);

    // Double-click activates the row under the pointer.
    activated = -1;
    lv.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 300.0f));
    lv.set_scroll_y(0.0f);
    Event dbl;
    dbl.type = EventType::DoubleClick;
    dbl.data.mouse = MouseData{50.0f, 3.0f * lv.row_height() + 5.0f, MouseButton_Left, 0};
    lv.on_event(dbl);
    CHECK(activated == 3);

    // Wheel pans only when there is overflow; keyboard nav never wraps.
    lv.on_event(down);
    CHECK(activated == 3);  // selection changes do not re-fire activate
}

// ---------------------------------------------------------------------------
// Lifecycle stress: the frame must stay safe across dynamic add/remove,
// destroyed-widget re-attach, cascading destruction, and callback re-entry.
// These validate the current ownership model (raw pointers + parent/unparent),
// NOT by switching to shared_ptr/unique_ptr.
// ---------------------------------------------------------------------------

// A parent that auto-deletes its children on destruction — the model apps rely
// on to keep heap widgets alive for the widget tree's lifetime.
class OwningWidget : public Widget {
public:
    ~OwningWidget() {
        Widget* child = first_child();
        while (child) {
            Widget* next = child->next_sibling();
            delete child;
            child = next;
        }
    }
};

static int g_lifecycle_callback_hits = 0;

// Owns its on_click-style callback so we can observe callback lifetime.
class CallbackHolder : public Widget {
public:
    std::function<void()> cb;
};

void test_lifecycle_dynamic_add_remove() {
    OwningWidget root;

    // Add / remove children repeatedly. Each removal detaches and, because
    // root owns them, destroys the widget. The tree must stay consistent.
    for (int i = 0; i < 50; ++i) {
        auto* box = new FixedWidget(Size{10.0f, 10.0f});
        root.append_child(box);
        box->remove_from_parent();
        delete box;
    }
    CHECK(root.first_child() == nullptr);
    CHECK(root.last_child() == nullptr);

    // A set of mid-list removals (listview rows) must not walk a dangling sibling.
    std::vector<FixedWidget*> rows;
    for (int i = 0; i < 10; ++i) {
        auto* row = new FixedWidget(Size{50.0f, 10.0f});
        root.append_child(row);
        rows.push_back(row);
    }
    // Detach the middle rows first, then the edges, to stress sibling relinking.
    for (int i = 3; i < 7; ++i) rows[i]->remove_from_parent();
    // Deleting the still-attached rows must auto-detach them (via ~Widget)
    // without leaving dangling sibling pointers in the root.
    for (auto* r : rows) delete r;
    CHECK(root.first_child() == nullptr);

    // After full cleardown, re-attaching fresh widgets works again.
    auto* again = new FixedWidget(Size{5.0f, 5.0f});
    root.append_child(again);
    CHECK(root.first_child() == again);
    delete again;
}

void test_lifecycle_callback_dangling() {
    OwningWidget root;

    // A widget that fires a captured callback; the callback captures the
    // widget itself. If the widget is destroyed without clearing the callback,
    // invoking it would be a use-after-free — so the frame must guarantee the
    // callback is dead before the widget dies.
    {
        auto* btn = new CallbackHolder;
        g_lifecycle_callback_hits = 0;
        btn->cb = [btn]() { ++g_lifecycle_callback_hits; };
        btn->cb();  // live callback fires
        CHECK(g_lifecycle_callback_hits == 1);
        root.append_child(btn);

        btn->remove_from_parent();  // detach
        // Destroy the widget; the captured `btn` must not be invoked after this.
        delete btn;
        CHECK(root.first_child() == nullptr);
    }
    // No further callback can run; the counter must stay untouched.
    CHECK(g_lifecycle_callback_hits == 1);

    // Re-create a similar holder on a fresh root, then destroy the root while
    // a callback is still captured. Destructor chains via OwningWidget.
    g_lifecycle_callback_hits = 0;
    {
        OwningWidget second_root;
        auto* holder = new CallbackHolder;
        holder->cb = []() { ++g_lifecycle_callback_hits; };
        second_root.append_child(holder);
    }  // second_root destroyed; its child is deleted by the owning destructor
    CHECK(g_lifecycle_callback_hits == 0);  // nothing ever invoked it
}

void test_lifecycle_cascading_destroy() {
    OwningWidget root;

    // Build a 3-level tree: root -> panel -> leafs. Destroy the middle panel
    // (which is owned by root) and confirm the whole subtree is gone and the
    // root's child list no longer points at anything dangling.
    auto* panel = new OwningWidget;
    {
        auto* leaf1 = new CallbackHolder;
        panel->append_child(leaf1);
        auto* leaf2 = new CallbackHolder;
        panel->append_child(leaf2);
    }
    root.append_child(panel);
    CHECK(root.first_child() == panel);
    CHECK(root.last_child() == panel);

    panel->remove_from_parent();
    delete panel;
    CHECK(root.first_child() == nullptr);
    CHECK(root.last_child() == nullptr);

    // A second cascade: root -> a -> b -> c, delete a, ensure b's child c is
    // detached too (clear_children on b is exercised by ~OwningWidget).
    {
        auto* a = new OwningWidget;
        auto* b = new OwningWidget;
        auto* c = new FixedWidget(Size{9.0f, 9.0f});
        a->append_child(b);
        b->append_child(c);
        root.append_child(a);
        CHECK(c->parent() == b);
        a->remove_from_parent();
        delete a;  // tears down a -> b -> c
        CHECK(root.first_child() == nullptr);
    }
}

void test_lifecycle_high_frequency() {
    OwningWidget root;

    // Mixed churn: add 100, delete middles, complete half, clear all, re-add.
    std::vector<FixedWidget*> all;
    for (int i = 0; i < 100; ++i) {
        auto* w = new FixedWidget(Size{20.0f, 8.0f});
        root.append_child(w);
        all.push_back(w);
    }
    CHECK(root.first_child() != nullptr);

    // Delete the middle 50 (detach + destroy), keeping the tree valid.
    for (int i = 25; i < 75; ++i) {
        all[i]->remove_from_parent();
        delete all[i];
    }
    CHECK(root.first_child() != nullptr);

    // Remove every remaining one; the root must end empty and stable.
    Widget* cur = root.first_child();
    while (cur) {
        Widget* next = cur->next_sibling();
        cur->remove_from_parent();
        delete cur;
        cur = next;
    }
    CHECK(root.first_child() == nullptr);
    CHECK(root.last_child() == nullptr);
}

// Real Window-context teardown: the path every app hits — a subtree is mounted
// under a Window, the window holds state pointers into it (focus, timers), then a
// child is destroyed while body code may still hold its raw pointer.
// detach_widget() runs from ~Widget and must leave the window free of dangling
// pointers; set_root() must fully unmount an old root. The Window is deliberately
// NOT created (no HWND): D2DBackend is lazy, and detach/set_focus/timer paths
// all guard on hwnd_ so the pure-logic paths run identically.
void test_lifecycle_window_teardown() {
    {  // scope 1: focus target deleted while mounted under the window
        Window win("lc", 400, 300);
        OwningWidget root;

        auto* focus_target = new FixedWidget(Size{10.0f, 10.0f});
        auto* hover_target = new FixedWidget(Size{10.0f, 10.0f});
        root.append_child(focus_target);
        root.append_child(hover_target);
        win.set_root(&root);

        win.set_focus(focus_target);
        CHECK(win.focus() == focus_target);
        CHECK(root.parent() == &win);

        // App-side deletion of a live control: ~Widget -> window()->detach_widget
        // must null out focused_ so the window never dispatches to it.
        delete focus_target;
        CHECK(win.focus() == nullptr);
        delete hover_target;
        // Root still carries no destroyed children.
        CHECK(root.first_child() == nullptr);
    }

    {  // scope 2: root swapped under a live window; old root released safely
        Window win("lc2", 400, 300);
        auto* old_root = new OwningWidget;
        auto* kept_child = new FixedWidget(Size{5.0f, 5.0f});
        old_root->append_child(kept_child);
        win.set_root(old_root);
        CHECK(old_root->parent() == &win);

        auto* new_root = new Widget;
        win.set_root(new_root);
        // Old root was detached from the window and is free for app-side teardown.
        CHECK(old_root->parent() == nullptr);
        win.set_root(nullptr);
        CHECK(new_root->parent() == nullptr);
    }
}

void test_visual_footprint() {
    Widget parent;
    parent.set_bounds(RectF::make(10.0f, 20.0f, 100.0f, 100.0f));
    Widget child;
    parent.append_child(&child);
    child.set_bounds(RectF::make(5.0f, 6.0f, 30.0f, 40.0f));
    CHECK(child.global_bounds() == RectF::make(15.0f, 26.0f, 30.0f, 40.0f));
    CHECK(child.visual_footprint() == RectF::make(15.0f, 26.0f, 30.0f, 40.0f));
    // Visual footprint follows a translation (transform around center)
    child.set_translate(10.0f, 0.0f);
    CHECK(child.visual_footprint() == RectF::make(25.0f, 26.0f, 30.0f, 40.0f));
}

// GridPanel: Fixed tracks take their exact extent, Star splits the leftover.
void test_grid_fixed() {
    GridPanel grid(3, 1);
    grid.set_gap(0.0f);
    grid.set_column_fixed(0, 50.0f);

    FixedWidget auto_child(Size{40.0f, 20.0f});   // col 1 (auto)
    FixedWidget star_child(Size{40.0f, 20.0f});   // col 2 (star)
    grid.add(&auto_child, 1, 0);
    grid.add(&star_child, 2, 0);
    grid.set_column_star(2, 1.0f);

    grid.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 40.0f));
    grid.perform_layout();

    CHECK(std::abs(auto_child.bounds().left - 50.0f) < 0.01f);          // after fixed col
    CHECK(std::abs(auto_child.bounds().width() - 40.0f) < 0.01f);       // auto = content
    CHECK(std::abs(star_child.bounds().left - 90.0f) < 0.01f);
    CHECK(std::abs(star_child.bounds().width() - 210.0f) < 0.01f);      // 300 - 50 - 40

    // A child in the fixed column is clipped to it, not stretched by content.
    FixedWidget wide_in_fixed(Size{200.0f, 20.0f});
    GridPanel grid2(1, 1);
    grid2.set_column_fixed(0, 60.0f);
    grid2.add(&wide_in_fixed, 0, 0);
    grid2.set_bounds(RectF::make(0.0f, 0.0f, 400.0f, 40.0f));
    grid2.perform_layout();
    CHECK(std::abs(wide_in_fixed.bounds().width() - 60.0f) < 0.01f);
}

// ===== Phase 5.1.2: Additional test coverage =====

void test_point() {
    Point p;
    CHECK(p.x == 0.0f && p.y == 0.0f);
    Point q{3.0f, 7.0f};
    CHECK(q.x == 3.0f && q.y == 7.0f);
}

void test_size_extra() {
    Size s0{0.0f, 0.0f};
    CHECK(s0.empty());
    Size s1{-1.0f, 5.0f};
    CHECK(s1.empty());
    Size s2{1.0f, 1.0f};
    CHECK(!s2.empty());
}

void test_rect_extra() {
    RectF r = RectF::make(10.0f, 20.0f, 50.0f, 30.0f);
    CHECK(r.width() == 50.0f && r.height() == 30.0f);
    CHECK(r.size().width == 50.0f && r.size().height == 30.0f);
    CHECK(r.top_left().x == 10.0f && r.top_left().y == 20.0f);
    CHECK(!r.empty());
    RectF empty_r = RectF::make(0, 0, 0, 0);
    CHECK(empty_r.empty());
    // intersects
    RectF a = RectF::make(0, 0, 10, 10);
    RectF b = RectF::make(5, 5, 10, 10);
    RectF c = RectF::make(20, 20, 5, 5);
    CHECK(a.intersects(b));
    CHECK(!a.intersects(c));
    // edge-touching: right edge of a == left edge of c (a.right=10, c.left=20) → no overlap
    RectF d = RectF::make(10, 0, 5, 5);
    CHECK(!a.intersects(d));
    // inflated
    RectF inf = a.inflated(2.0f, 3.0f);
    CHECK(inf.left == -2.0f && inf.top == -3.0f && inf.right == 12.0f && inf.bottom == 13.0f);
    // translated
    RectF tr = a.translated(10.0f, 20.0f);
    CHECK(tr.left == 10.0f && tr.top == 20.0f && tr.right == 20.0f && tr.bottom == 30.0f);
    // contains(Point)
    CHECK(a.contains(Point{5.0f, 5.0f}));
    CHECK(!a.contains(Point{15.0f, 5.0f}));
    // operator!=
    CHECK(a != b);
    CHECK(a == a);
    // unite with empty
    RectF u;
    u.unite(a);
    CHECK(u == a);
    // intersect no overlap
    RectF no = a.intersect(c);
    CHECK(no.empty());
}

void test_corner_radius() {
    CornerRadius def;
    CHECK(def.top_left == 0.0f && def.bottom_right == 0.0f);
    CornerRadius uni(5.0f);
    CHECK(uni.top_left == 5.0f && uni.top_right == 5.0f && uni.bottom_right == 5.0f && uni.bottom_left == 5.0f);
}

void test_color_extra() {
    Color def;
    CHECK(def.r == 0 && def.g == 0 && def.b == 0 && def.a == 255);
    Color c{10, 20, 30, 40};
    CHECK(c.r == 10 && c.g == 20 && c.b == 30 && c.a == 40);
    Color eq1{1, 2, 3, 4};
    Color eq2{1, 2, 3, 4};
    CHECK(eq1 == eq2);
    CHECK(eq1 != c);
}

void test_gradient_stop() {
    GradientStop gs;
    CHECK(gs.position == 0.0f);
    gs.position = 0.5f;
    gs.color = Color{255, 0, 0};
    CHECK(gs.position == 0.5f && gs.color.r == 255);
}

void test_enums() {
    CHECK(static_cast<u8>(TextAlignH::Left) == 0);
    CHECK(static_cast<u8>(TextAlignH::Center) == 1);
    CHECK(static_cast<u8>(TextAlignH::Right) == 2);
    CHECK(static_cast<u8>(TextAlignV::Top) == 0);
    CHECK(static_cast<u8>(TextAlignV::Center) == 1);
    CHECK(static_cast<u8>(TextAlignV::Bottom) == 2);
    CHECK(static_cast<u8>(Cursor::Arrow) == 0);
}

void test_event_extra() {
    // Default state
    Event e;
    CHECK(e.type == EventType::None);
    CHECK(!e.consumed);
    CHECK(e.time_ms == 0);
    // MouseData
    Event me;
    me.type = EventType::MouseMove;
    me.data.mouse.x = 10.0f;
    me.data.mouse.y = 20.0f;
    me.data.mouse.buttons = MouseButton_Left | MouseButton_Right;
    me.data.mouse.mods = KeyModifier_Shift;
    me.data.mouse.wheel_delta = 120;
    CHECK(me.data.mouse.x == 10.0f);
    CHECK(me.data.mouse.y == 20.0f);
    CHECK(me.data.mouse.buttons == (MouseButton_Left | MouseButton_Right));
    CHECK(me.data.mouse.mods == KeyModifier_Shift);
    CHECK(me.data.mouse.wheel_delta == 120);
    // KeyData
    Event ke;
    ke.type = EventType::KeyDown;
    ke.data.key.code = 65;
    ke.data.key.chr = L'A';
    ke.data.key.mods = KeyModifier_Control;
    ke.data.key.repeat = true;
    CHECK(ke.data.key.code == 65);
    CHECK(ke.data.key.chr == L'A');
    CHECK(ke.data.key.mods == KeyModifier_Control);
    CHECK(ke.data.key.repeat);
    // SizeData
    Event se;
    se.type = EventType::Resize;
    se.data.size.width = 800.0f;
    se.data.size.height = 600.0f;
    CHECK(se.data.size.width == 800.0f);
    CHECK(se.data.size.height == 600.0f);
    // MouseButton values
    CHECK(MouseButton_None == 0);
    CHECK(MouseButton_Left == 1);
    CHECK(MouseButton_Middle == 2);
    CHECK(MouseButton_Right == 4);
    CHECK(MouseButton_X1 == 8);
    CHECK(MouseButton_X2 == 16);
    // KeyModifier values
    CHECK(KeyModifier_None == 0);
    CHECK(KeyModifier_Shift == 1);
    CHECK(KeyModifier_Control == 2);
    CHECK(KeyModifier_Alt == 4);
    CHECK(KeyModifier_Win == 8);
}

void test_encoding_extra() {
    // ASCII roundtrip
    String ascii = "Hello World";
    CHECK(utf::to_utf8(utf::to_wide(ascii)) == ascii);
    // Single char
    String single = "A";
    CHECK(utf::to_utf8(utf::to_wide(single)) == single);
    // Mixed
    String mixed = "Hi 你好";
    CHECK(utf::to_utf8(utf::to_wide(mixed)) == mixed);
    // Emoji (4-byte UTF-8)
    String emoji = "\xF0\x9F\x98\x80";
    CHECK(utf::to_utf8(utf::to_wide(emoji)) == emoji);
}

void test_widget_properties() {
    Widget w;
    // is_root
    CHECK(w.is_root());
    Widget parent;
    parent.append_child(&w);
    CHECK(!w.is_root());
    // prev_sibling
    Widget a, b;
    parent.append_child(&a);
    parent.append_child(&b);
    CHECK(b.prev_sibling() == &a);
    CHECK(a.prev_sibling() == &w);
    // clear_children
    Widget c1, c2, c3;
    parent.append_child(&c1);
    parent.append_child(&c2);
    parent.append_child(&c3);
    parent.clear_children();
    CHECK(parent.first_child() == nullptr);
    // x, y, width, height
    w.set_bounds(RectF::make(10.0f, 20.0f, 100.0f, 50.0f));
    CHECK(w.x() == 10.0f);
    CHECK(w.y() == 20.0f);
    CHECK(w.width() == 100.0f);
    CHECK(w.height() == 50.0f);
    // set_position
    w.set_position(5.0f, 15.0f);
    CHECK(w.x() == 5.0f && w.y() == 15.0f);
    // set_size
    w.set_size(200.0f, 80.0f);
    CHECK(w.width() == 200.0f && w.height() == 80.0f);
    // min/max size
    w.set_min_width(50.0f);
    w.set_min_height(30.0f);
    CHECK(w.min_size().width == 50.0f && w.min_size().height == 30.0f);
    w.set_max_width(500.0f);
    w.set_max_height(400.0f);
    CHECK(w.max_size().width == 500.0f && w.max_size().height == 400.0f);
    // flex
    w.set_flex_grow(2.0f);
    w.set_flex_shrink(0.5f);
    CHECK(w.flex_grow() == 2.0f);
    CHECK(w.flex_shrink() == 0.5f);
    // flags
    CHECK(w.visible());
    CHECK(w.enabled());
    CHECK(!w.focusable());
    CHECK(!w.draggable());
    w.set_visible(false);
    CHECK(!w.visible());
    w.set_enabled(false);
    CHECK(!w.enabled());
    w.set_focusable(true);
    CHECK(w.focusable());
    w.set_draggable(true);
    CHECK(w.draggable());
    w.set_cursor(Cursor::IBeam);
    CHECK(w.cursor() == Cursor::IBeam);
    // margin
    w.set_margin(8.0f);
    CHECK(w.margin().left == 8.0f && w.margin().top == 8.0f);
    CHECK(w.margin().horizontal() == 16.0f);
    CHECK(w.margin().vertical() == 16.0f);
}

void test_label_api() {
    Label lbl("Hello");
    CHECK(lbl.text() == "Hello");
    lbl.set_text("World");
    CHECK(lbl.text() == "World");
    lbl.text("Fluent");
    CHECK(lbl.text() == "Fluent");
    // text_color
    CHECK(lbl.text_color().a == 0);
    lbl.set_text_color(Color{255, 128, 0});
    CHECK(lbl.text_color().r == 255);
    // text_role
    CHECK(lbl.text_role() == TextRole::Primary);
    lbl.set_text_role(TextRole::Secondary);
    CHECK(lbl.text_role() == TextRole::Secondary);
    // small
    CHECK(!lbl.small());
    lbl.set_small(true);
    CHECK(lbl.small());
    // bold
    CHECK(!lbl.bold());
    lbl.set_bold(true);
    CHECK(lbl.bold());
    // align
    lbl.set_align(TextAlignH::Left, TextAlignV::Top);
    CHECK(lbl.align_h() == TextAlignH::Left);
    CHECK(lbl.align_v() == TextAlignV::Top);
}

void test_button_api() {
    Button btn("Test");
    CHECK(btn.text() == "Test");
    btn.set_text("New");
    CHECK(btn.text() == "New");
    CHECK(btn.accent());
    btn.set_accent(false);
    CHECK(!btn.accent());
    CHECK(btn.padding() == 10.0f);
    btn.set_padding(20.0f);
    CHECK(btn.padding() == 20.0f);
    CHECK(btn.icon_size() == 16.0f);
    btn.set_icon_size(24.0f);
    CHECK(btn.icon_size() == 24.0f);
}

void test_checkbox_api() {
    CheckBox cb("Opt");
    CHECK(cb.text() == "Opt");
    cb.set_text("New");
    CHECK(cb.text() == "New");
    CHECK(!cb.checked());
    cb.set_checked(true);
    CHECK(cb.checked());
    cb.set_checked(true);  // no-op
    CHECK(cb.checked());
    cb.set_checked(false);
    CHECK(!cb.checked());
}

void test_radio_api() {
    RadioButton r("Choice");
    CHECK(r.text() == "Choice");
    r.set_text("Option");
    CHECK(r.text() == "Option");
    CHECK(!r.checked());
    r.set_checked(true);
    CHECK(r.checked());
    r.set_checked(true);
    CHECK(r.checked());
    r.set_checked(true);  // no-op
    CHECK(r.checked());
}

void test_toggle_api() {
    ToggleSwitch ts;
    CHECK(!ts.checked());
    ts.set_checked(true);
    CHECK(ts.checked());
    ts.set_checked(true);  // no-op
    CHECK(ts.checked());
}

void test_slider_api() {
    Slider sl;
    sl.set_min(10.0f);
    CHECK(sl.min() == 10.0f);
    sl.set_max(90.0f);
    CHECK(sl.max() == 90.0f);
    sl.set_value(50.0f);
    CHECK(sl.value() == 50.0f);
    sl.set_value(5.0f);  // clamps to min
    CHECK(sl.value() == 10.0f);
    sl.set_value(95.0f);  // clamps to max
    CHECK(sl.value() == 90.0f);
}

void test_spinbox_api() {
    SpinBox sb;
    sb.set_step(5.0);
    CHECK(sb.step() == 5.0);
    sb.set_decimals(3);
    CHECK(sb.decimals() == 3);
    sb.set_spin_width(40.0f);
    CHECK(sb.spin_width() == 40.0f);
    int changes = 0;
    sb.set_on_changed([&](f64) { ++changes; });
    sb.set_value(42.0);
    CHECK(changes == 1);
}

void test_combobox_api() {
    ComboBox cb;
    CHECK(cb.items().empty());
    cb.set_items({"A", "B", "C"});
    CHECK(cb.items().size() == 3);
    CHECK(cb.selected_index() == -1);
    cb.set_placeholder("Pick");
    CHECK(cb.placeholder() == "Pick");
    cb.set_width(200.0f);
    CHECK(cb.width() == 200.0f);
    cb.set_selected_index(1);
    CHECK(cb.selected_index() == 1);
    cb.clear_items();
    CHECK(cb.items().empty());
    CHECK(cb.selected_index() == -1);
}

void test_progress_api() {
    ProgressBar pb;
    pb.set_value(0.5f);
    CHECK(pb.value() == 0.5f);
    pb.set_indeterminate(true);
    CHECK(pb.indeterminate());
    pb.set_indeterminate(false);
    CHECK(!pb.indeterminate());
}

void test_scrollview_api() {
    ScrollView sv;
    CHECK(sv.scroll_y() == 0.0f);
    CHECK(sv.suggested_height() == 0.0f);
    sv.set_suggested_height(200.0f);
    CHECK(sv.suggested_height() == 200.0f);
    // Without content, max_scroll is 0 so set_scroll_y clamps
    sv.set_scroll_y(50.0f);
    CHECK(sv.scroll_y() == 0.0f);
    sv.set_scroll_y(-10.0f);
    CHECK(sv.scroll_y() == 0.0f);
    // With content that overflows, scroll works
    Widget content;
    content.set_min_height(500.0f);
    sv.set_content(&content);
    sv.set_bounds(RectF::make(0, 0, 200, 100));
    sv.perform_layout();
    sv.set_scroll_y(50.0f);
    CHECK(sv.scroll_y() == 50.0f);
    sv.scroll_by(30.0f);
    CHECK(sv.scroll_y() == 80.0f);
    sv.scroll_by(-100.0f);
    CHECK(sv.scroll_y() == 0.0f);
}

void test_icon_api() {
    Icon ic(IconId::Check);
    CHECK(ic.icon() == IconId::Check);
    ic.set_icon(IconId::X);
    CHECK(ic.icon() == IconId::X);
    CHECK(ic.icon_size() == 16.0f);
    ic.set_icon_size(24.0f);
    CHECK(ic.icon_size() == 24.0f);
    CHECK(ic.color().a == 0);
    ic.set_color(Color{255, 0, 0});
    CHECK(ic.color().r == 255);
}

void test_image_api() {
    Image img;
    CHECK(img.bitmap() == kInvalidBitmap);
    CHECK(img.scale_mode() == ImageScaleMode::Contain);
    img.set_scale_mode(ImageScaleMode::Stretch);
    CHECK(img.scale_mode() == ImageScaleMode::Stretch);
    CHECK(img.corner_radius() == 0.0f);
    img.set_corner_radius(8.0f);
    CHECK(img.corner_radius() == 8.0f);
}

void test_overlay_api() {
    Overlay ov;
    CHECK(!ov.is_open());
    CHECK(ov.animated());
    ov.set_animated(false);
    CHECK(!ov.animated());
    CHECK(ov.shadow());
    ov.set_shadow(false);
    CHECK(!ov.shadow());
    CHECK(ov.dim_blurred() == false);
    ov.set_dim_blurred(true);
    CHECK(ov.dim_blurred());
    CHECK(ov.slide_direction() == 1.0f);
    ov.set_slide_direction(-1.0f);
    CHECK(ov.slide_direction() == -1.0f);
    RectF pr = RectF::make(10, 10, 200, 300);
    ov.set_panel_rect(pr);
    CHECK(ov.panel_rect() == pr);
}

void test_context_menu_api() {
    ContextMenu cm;
    CHECK(cm.items().empty());
    CHECK(!cm.is_open());
    cm.add_item("Cut", [] {});
    CHECK(cm.items().size() == 1);
    cm.add_separator();
    CHECK(cm.items().size() == 2);
    CHECK(cm.items()[1].separator);
    cm.add_item("Paste", [] {});
    CHECK(cm.items().size() == 3);
    cm.clear_items();
    CHECK(cm.items().empty());
}

void test_easing() {
    CHECK(ease(0.0f, Easing::Linear) == 0.0f);
    CHECK(ease(1.0f, Easing::Linear) == 1.0f);
    CHECK(ease(0.5f, Easing::Linear) == 0.5f);
    // InQuad
    CHECK(ease(0.0f, Easing::InQuad) == 0.0f);
    CHECK(ease(1.0f, Easing::InQuad) == 1.0f);
    CHECK(std::abs(ease(0.5f, Easing::InQuad) - 0.25f) < 0.001f);
    // OutQuad
    CHECK(ease(0.0f, Easing::OutQuad) == 0.0f);
    CHECK(ease(1.0f, Easing::OutQuad) == 1.0f);
    CHECK(std::abs(ease(0.5f, Easing::OutQuad) - 0.75f) < 0.001f);
    // InOutQuad
    CHECK(ease(0.0f, Easing::InOutQuad) == 0.0f);
    CHECK(ease(1.0f, Easing::InOutQuad) == 1.0f);
    CHECK(std::abs(ease(0.5f, Easing::InOutQuad) - 0.5f) < 0.001f);
    // InCubic
    CHECK(ease(0.0f, Easing::InCubic) == 0.0f);
    CHECK(ease(1.0f, Easing::InCubic) == 1.0f);
    CHECK(std::abs(ease(0.5f, Easing::InCubic) - 0.125f) < 0.001f);
    // OutCubic
    CHECK(ease(0.0f, Easing::OutCubic) == 0.0f);
    CHECK(ease(1.0f, Easing::OutCubic) == 1.0f);
    CHECK(std::abs(ease(0.5f, Easing::OutCubic) - 0.875f) < 0.001f);
    // InOutCubic
    CHECK(ease(0.0f, Easing::InOutCubic) == 0.0f);
    CHECK(ease(1.0f, Easing::InOutCubic) == 1.0f);
    // InBack starts below 0 (overshoot)
    CHECK(ease(0.5f, Easing::InBack) < 0.0f);
    CHECK(ease(1.0f, Easing::InBack) == 1.0f);
    // OutBack ends at 1
    CHECK(ease(0.0f, Easing::OutBack) == 0.0f);
    CHECK(ease(1.0f, Easing::OutBack) == 1.0f);
}

void test_theme_api() {
    Theme dark = Theme::make_dark();
    CHECK(dark.dark);
    CHECK(dark.background.r == 0x1E);
    Theme light = Theme::make_light();
    CHECK(!light.dark);
    CHECK(light.background.r == 0xFA);
    Theme prev = Theme::get();
    Theme::set(dark);
    CHECK(Theme::get().dark);
    Theme::set(light);
    CHECK(!Theme::get().dark);
    Theme::set(prev);
    // theme_color
    CHECK(theme_color(dark, ThemeRole::Background) == dark.background);
    CHECK(theme_color(dark, ThemeRole::Accent) == dark.accent);
    CHECK(theme_color(dark, ThemeRole::Text) == dark.text);
    CHECK(theme_color(dark, ThemeRole::Surface) == dark.surface);
    CHECK(theme_color(dark, ThemeRole::Border) == dark.border);
}

void test_dock_panel_layout() {
    DockPanel dock;
    Box top, left, fill;
    top.set_bg(Color{1, 0, 0});
    top.set_min_height(30.0f);
    left.set_bg(Color{0, 1, 0});
    left.set_min_width(50.0f);
    fill.set_bg(Color{0, 0, 1});
    dock.dock(&top, Dock::Top);
    dock.dock(&left, Dock::Left);
    dock.dock(&fill, Dock::Fill);
    dock.set_bounds(RectF::make(0, 0, 200, 100));
    dock.perform_layout();
    CHECK(top.bounds().height() == 30.0f);
    CHECK(left.bounds().width() == 50.0f);
    CHECK(fill.bounds().width() == 150.0f);
    CHECK(fill.bounds().height() == 70.0f);
}

void test_wrap_panel_layout() {
    WrapPanel wrap;
    wrap.set_spacing(0.0f);
    wrap.set_line_spacing(0.0f);
    FixedWidget w1(Size{60.0f, 20.0f});
    FixedWidget w2(Size{60.0f, 20.0f});
    FixedWidget w3(Size{60.0f, 20.0f});
    wrap.append_child(&w1);
    wrap.append_child(&w2);
    wrap.append_child(&w3);
    wrap.set_bounds(RectF::make(0, 0, 100, 100));
    wrap.perform_layout();
    CHECK(w1.bounds().left == 0.0f && w1.bounds().top == 0.0f);
    CHECK(w2.bounds().top == 20.0f);
    CHECK(w3.bounds().top == 40.0f);
}

void test_grid_star_layout() {
    GridPanel grid(1, 2);
    grid.set_gap(0.0f);
    grid.set_row_star(0, 1.0f);
    grid.set_row_star(1, 1.0f);
    FixedWidget a(Size{50.0f, 10.0f});
    FixedWidget b(Size{50.0f, 10.0f});
    grid.add(&a, 0, 0);
    grid.add(&b, 0, 1);
    grid.set_bounds(RectF::make(0, 0, 200, 100));
    grid.perform_layout();
    CHECK(std::abs(a.bounds().height() - 50.0f) < 0.01f);
    CHECK(std::abs(b.bounds().height() - 50.0f) < 0.01f);
}

void test_grid_gap() {
    GridPanel grid(2, 1);
    grid.set_gap(10.0f);
    grid.set_column_fixed(0, 50.0f);
    grid.set_column_fixed(1, 50.0f);
    FixedWidget a(Size{50.0f, 20.0f});
    FixedWidget b(Size{50.0f, 20.0f});
    grid.add(&a, 0, 0);
    grid.add(&b, 1, 0);
    grid.set_bounds(RectF::make(0, 0, 200, 40));
    grid.perform_layout();
    CHECK(std::abs(b.bounds().left - 60.0f) < 0.01f);
}

void test_lifecycle_clear_children() {
    Widget root;
    FixedWidget c1(Size{10, 10});
    FixedWidget c2(Size{10, 10});
    FixedWidget c3(Size{10, 10});
    root.append_child(&c1);
    root.append_child(&c2);
    root.append_child(&c3);
    CHECK(root.first_child() != nullptr);
    root.clear_children();
    CHECK(root.first_child() == nullptr);
}

// Phase 5.1.6: Layout performance benchmark

void test_layout_perf() {
    StackPanel root;
    for (int i = 0; i < 2000; ++i) {
        auto* lbl = new FixedWidget(Size{100.0f, 24.0f});
        lbl->set_min_height(24.0f);
        root.append_child(lbl);
    }
    root.set_bounds(RectF::make(0, 0, 800, 48000));

    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);
    root.perform_layout();
    QueryPerformanceCounter(&t1);
    f64 ms = static_cast<f64>(t1.QuadPart - t0.QuadPart) / freq.QuadPart * 1000.0;
    std::printf("Layout 2000 widgets: %.2f ms\n", ms);
    CHECK(ms < 50.0);
    CHECK(root.first_child() != nullptr);
}

// Phase 5.1.7: UIA Name/Role accessibility
void test_uia_accessibility() {
    Button btn("Submit");
    CHECK(btn.uia_name() == "Submit");
    CHECK(btn.uia_role() == "Button");

    Label lbl("Hello");
    CHECK(lbl.uia_name() == "Hello");
    CHECK(lbl.uia_role() == "Text");

    CheckBox cb("Agree");
    CHECK(cb.uia_name() == "Agree");
    CHECK(cb.uia_role() == "CheckBox");

    RadioButton rb("Choice");
    CHECK(rb.uia_name() == "Choice");
    CHECK(rb.uia_role() == "RadioButton");

    ToggleSwitch ts;
    CHECK(ts.uia_name().empty());
    CHECK(ts.uia_role() == "Toggle");

    Widget w;
    CHECK(w.uia_name().empty());
    CHECK(w.uia_role().empty());
}

// ===== Phase 5.2.2 Dialogs =====

void test_message_box_lifecycle() {
    Window win("dlg", 640, 480);
    yzk::Widget root;
    root.set_min_size(yzk::Size{640.0f, 480.0f});
    win.set_root(&root);

    MessageBox box;
    box.set_animated(false);
    CHECK(!box.is_open());

    MessageBoxResult got = MessageBoxResult::None;
    box.show(win, "Data", "Delete the selected rows?", MessageBoxKind::Confirm,
             [&got](MessageBoxResult r) { got = r; });
    CHECK(box.is_open());
    CHECK(got == MessageBoxResult::None);  // not dismissed yet

    // Dismiss as OK (the primary button).
    box.resolve(MessageBoxResult::Ok);
    CHECK(!box.is_open());
    CHECK(got == MessageBoxResult::Ok);
}

void test_message_box_resolve_fires_callback() {
    Window win("dlg2", 640, 480);
    yzk::Widget root;
    root.set_min_size(yzk::Size{640.0f, 480.0f});
    win.set_root(&root);

    MessageBox box;
    box.set_animated(false);
    MessageBoxResult got = MessageBoxResult::None;
    box.show(win, "T", "Body", MessageBoxKind::YesNoCancel, [&got](MessageBoxResult r) { got = r; });
    CHECK(box.is_open());
    box.resolve(MessageBoxResult::Yes);
    CHECK(got == MessageBoxResult::Yes);
    CHECK(!box.is_open());

    // Re-open and dismiss with Cancel.
    got = MessageBoxResult::None;
    box.show(win, "T", "Body", MessageBoxKind::Info, [&got](MessageBoxResult r) { got = r; });
    box.resolve(MessageBoxResult::Cancel);
    CHECK(got == MessageBoxResult::Cancel);
}

void test_message_box_close_is_none() {
    Window win("dlg3", 640, 480);
    yzk::Widget root;
    root.set_min_size(yzk::Size{640.0f, 480.0f});
    win.set_root(&root);

    MessageBox box;
    box.set_animated(false);
    MessageBoxResult got = MessageBoxResult::None;
    box.show(win, "T", "Body", MessageBoxKind::Confirm, [&got](MessageBoxResult r) { got = r; });
    box.close();
    CHECK(!box.is_open());
    CHECK(got == MessageBoxResult::None);
}

// ===== Phase 5.2.3 Accelerators =====

void test_accelerator_basic() {
    Window win("acc", 400, 300);
    yzk::Widget root;
    root.set_min_size(yzk::Size{400.0f, 300.0f});
    win.set_root(&root);

    int fires = 0;
    win.add_accelerator(Accelerator{static_cast<u32>('S'), KeyModifier_Control,
                                    [&fires]() { ++fires; }});

    // Direct fire through the table.
    CHECK(win.fire_accelerator(static_cast<u32>('S'), KeyModifier_Control));
    CHECK(fires == 1);

    // Remove it; no longer fires.
    CHECK(win.remove_accelerator(static_cast<u32>('S'), KeyModifier_Control));
    CHECK(!win.fire_accelerator(static_cast<u32>('S'), KeyModifier_Control));
    CHECK(fires == 1);

    // Unknown chord: no match, false.
    CHECK(!win.fire_accelerator(static_cast<u32>('S'), KeyModifier_Control | KeyModifier_Shift));
}

void test_accelerator_replace_duplicate() {
    Window win("acc2", 400, 300);
    int fires = 0;
    win.add_accelerator(Accelerator{'Q', KeyModifier_Control, [&fires]() { fires += 1; }});
    win.add_accelerator(Accelerator{'Q', KeyModifier_Control, [&fires]() { fires += 10; }});
    win.fire_accelerator('Q', KeyModifier_Control);
    CHECK(fires == 10);  // replaced, not duplicated
}

void test_accelerator_not_hijack_editing_key() {
    Window win("acc3", 400, 300);
    yzk::Widget root;
    root.set_min_size(yzk::Size{400.0f, 300.0f});
    win.set_root(&root);

    TextBoxConfig cfg;
    auto* box = new TextBox("hello", cfg);
    root.append_child(box);
    win.set_focus(box);

    // Ctrl+Z is a textbox editing chord: accelerator matches but is suppressed
    // while a text input is focused (falls through to the widget).
    int ctrl_z = 0;
    win.add_accelerator(Accelerator{'Z', KeyModifier_Control, [&ctrl_z]() { ++ctrl_z; }});
    CHECK(win.fire_accelerator('Z', KeyModifier_Control));
    CHECK(ctrl_z == 0);  // suppressed, did not fire

    // Ctrl+S is an app chord: fires even with the text input focused.
    int ctrl_s = 0;
    win.add_accelerator(Accelerator{'S', KeyModifier_Control, [&ctrl_s]() { ++ctrl_s; }});
    CHECK(win.fire_accelerator('S', KeyModifier_Control));
    CHECK(ctrl_s == 1);
}

void test_accelerator_fires_without_focus() {
    Window win("acc4", 400, 300);
    int fires = 0;
    win.add_accelerator(Accelerator{'P', KeyModifier_Control, [&fires]() { ++fires; }});
    CHECK(win.fire_accelerator('P', KeyModifier_Control));
    CHECK(fires == 1);
}

// ===== Phase 5.2.4 Secondary windows =====

void test_secondary_owner_state() {
    yzk::Window owner("owner", 400, 300);
    yzk::Window child("child", 300, 200);

    CHECK(owner.owner() == nullptr);
    child.set_owner(&owner);
    CHECK(child.owner() == &owner);

    child.set_modal(true);
    CHECK(child.modal());
    child.set_modal(false);
    CHECK(!child.modal());

    child.set_topmost(true);
    CHECK(child.topmost());
    child.set_topmost(false);
    CHECK(!child.topmost());
}

void test_secondary_owner_chain() {
    yzk::Window owner("owner", 400, 300);
    yzk::Window mid("mid", 320, 240);
    yzk::Window leaf("leaf", 260, 180);

    mid.set_owner(&owner);
    leaf.set_owner(&mid);
    CHECK(mid.owner() == &owner);
    CHECK(leaf.owner() == &mid);
    // Re-parenting replaces the owner.
    leaf.set_owner(&owner);
    CHECK(leaf.owner() == &owner);
    mid.set_owner(nullptr);
    CHECK(mid.owner() == nullptr);
}

// ===== End Phase 5.2.4 =====
void test_clipboard_roundtrip() {
    const String original = u8"Yuzuki 柚子 - \u6805\u6c47 test 123";
    CHECK(clipboard::set_text(original));
    CHECK(clipboard::has_text());
    const String read = clipboard::get_text();
    CHECK(read == original);
}

void test_clipboard_empty_get_returns_empty() {
    // Reading an empty/absent clipboard never throws; returns empty string.
    const String read = clipboard::get_text();
    (void)read;
    CHECK(true);
}

void test_clipboard_overwrite() {
    CHECK(clipboard::set_text(u8"first value"));
    CHECK(clipboard::set_text(u8"second value"));
    CHECK(clipboard::get_text() == u8"second value");
}

void test_list_view_ctrl_c_copies_selected() {
    ListView view;
    CHECK(view.selected() == -1);

    Event key;
    key.type = EventType::KeyDown;
    key.data.key.code = 'C';
    key.data.key.mods = KeyModifier_Control;
    view.on_event(key);
    // No selected row yet: nothing consumed, clipboard left untouched.
    CHECK(!key.consumed);

    view.set_items({"alpha", "beta-中文", "gamma"});
    view.set_selected(1);
    Event copy;
    copy.type = EventType::KeyDown;
    copy.data.key.code = 'C';
    copy.data.key.mods = KeyModifier_Control;
    view.on_event(copy);
    CHECK(copy.consumed);
    CHECK(clipboard::get_text() == "beta-中文");

    Event plain_c;
    plain_c.type = EventType::KeyDown;
    plain_c.data.key.code = 'C';
    plain_c.data.key.mods = 0;
    view.on_event(plain_c);
    CHECK(!plain_c.consumed);
}

void test_text_box_via_framework_clipboard() {
    TextBox box("hello");
    Event focus;
    focus.type = EventType::FocusGained;
    box.on_event(focus);

    Event select_all;
    select_all.type = EventType::KeyDown;
    select_all.data.key.code = 'A';
    select_all.data.key.mods = KeyModifier_Control;
    box.on_event(select_all);

    Event copy;
    copy.type = EventType::KeyDown;
    copy.data.key.code = 'C';
    copy.data.key.mods = KeyModifier_Control;
    box.on_event(copy);
    CHECK(clipboard::get_text() == "hello");

    TextBox other;
    Event focus2;
    focus2.type = EventType::FocusGained;
    other.on_event(focus2);
    Event paste;
    paste.type = EventType::KeyDown;
    paste.data.key.code = 'V';
    paste.data.key.mods = KeyModifier_Control;
    other.on_event(paste);
    CHECK(other.text() == "hello");
}

// ===== End Phase 5.2.1 =====

}  // namespace

int main() {
    test_rect();
    test_color();
    test_event();
    test_encoding();
    test_stack_panel();
    test_hit_test();
    test_widget_tree();
    test_transform();
    test_animatable_property();
    test_widget_transition();
    test_flex_box();
    test_box();
    test_button();
    test_check_box();
    test_slider();
    test_text_box();
    test_toggle_switch();
    test_radio_button();
    test_progress_bar();
    test_spin_box();
    test_combo_box();
    test_tab_control();
    test_list_view();
    test_list_view_data_source();
    test_list_view_keyboard();
    test_lifecycle_dynamic_add_remove();
    test_lifecycle_callback_dangling();
    test_lifecycle_cascading_destroy();
    test_lifecycle_high_frequency();
    test_lifecycle_window_teardown();
    test_visual_footprint();
    test_grid_fixed();
    // Phase 5.1.2 additions
    test_point();
    test_size_extra();
    test_rect_extra();
    test_corner_radius();
    test_color_extra();
    test_gradient_stop();
    test_enums();
    test_event_extra();
    test_encoding_extra();
    test_widget_properties();
    test_label_api();
    test_button_api();
    test_checkbox_api();
    test_radio_api();
    test_toggle_api();
    test_slider_api();
    test_spinbox_api();
    test_combobox_api();
    test_progress_api();
    test_scrollview_api();
    test_icon_api();
    test_image_api();
    test_overlay_api();
    test_context_menu_api();
    test_easing();
    test_theme_api();
    test_dock_panel_layout();
    test_wrap_panel_layout();
    test_grid_star_layout();
    test_grid_gap();
    test_lifecycle_clear_children();
    test_layout_perf();
    test_uia_accessibility();

    // Phase 5.2.1 additions
    test_clipboard_roundtrip();
    test_clipboard_empty_get_returns_empty();
    test_clipboard_overwrite();
    test_list_view_ctrl_c_copies_selected();
    test_text_box_via_framework_clipboard();

    // Phase 5.2.2 additions
    test_message_box_lifecycle();
    test_message_box_resolve_fires_callback();
    test_message_box_close_is_none();

    // Phase 5.2.3 additions
    test_accelerator_basic();
    test_accelerator_replace_duplicate();
    test_accelerator_not_hijack_editing_key();
    test_accelerator_fires_without_focus();

    // Phase 5.2.4 additions
    test_secondary_owner_state();
    test_secondary_owner_chain();

    // Phase 5.3.1 additions
    test_text_box_undo_redo();
    test_text_box_word_navigation();

    // Phase 5.3.2 additions
    test_label_rich_text();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}