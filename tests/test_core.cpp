#include <yuzuki/yuzuki.hpp>

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
    // Content area = bounds minus padding
    CHECK(box.content_area() == RectF::make(15.0f, 15.0f, 80.0f, 40.0f));
    // Child fills the content area
    CHECK(child.bounds() == RectF::make(15.0f, 15.0f, 80.0f, 40.0f));
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

    // Multiline: Enter inserts a newline
    TextBoxConfig cfg;
    cfg.mode = TextBoxMode::MultiLine;
    TextBox ml("ab", cfg);
    Event focus3;
    focus3.type = EventType::FocusGained;
    ml.on_event(focus3);
    Event enter;
    enter.type = EventType::Character;
    enter.data.key.chr = L'\r';
    ml.on_event(enter);
    CHECK(ml.text() == "ab\n");
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
    CHECK(s.minimum() == 20.0 && s.maximum() == 80.0);
    CHECK(s.value() == 20.0);
    s.set_decimals(2);
    CHECK(s.text() == "20.00");
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
    test_list_view();
    test_list_view_data_source();
    test_visual_footprint();

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}