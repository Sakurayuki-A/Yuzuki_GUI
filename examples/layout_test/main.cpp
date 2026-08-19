#include <yuzuki/yuzuki.hpp>
#include <yuzuki/ui/application.hpp>

#include <windows.h>

#include <memory>
#include <string>

using namespace yzk;

namespace {

int g_failures = 0;
int g_checks = 0;

void check(bool condition, const char* file, int line, const char* expr) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::printf("FAIL %s:%d  %s\n", file, line, expr);
        if (FILE* f = std::fopen("C:/Users/p1590/AppData/Local/Temp/opencode/layout_test_result.txt", "a")) {
            std::fprintf(f, "FAIL %s:%d  %s\n", file, line, expr);
            std::fclose(f);
        }
    }
}

#define CHECK(expr) check((expr), __FILE__, __LINE__, #expr)

void check_near(f32 a, f32 b, const char* file, int line, const char* expr) {
    ++g_checks;
    if (a < b - 0.01f || a > b + 0.01f) {
        ++g_failures;
        std::printf("FAIL %s:%d  %s (got %f want %f)\n", file, line, expr,
                    static_cast<double>(a), static_cast<double>(b));
        if (FILE* f = std::fopen("C:/Users/p1590/AppData/Local/Temp/opencode/layout_test_result.txt", "a")) {
            std::fprintf(f, "FAIL %s:%d  %s (got %f want %f)\n", file, line, expr,
                         static_cast<double>(a), static_cast<double>(b));
            std::fclose(f);
        }
    }
}

#define CHECK_NEAR(a, b) check_near((a), (b), __FILE__, __LINE__, #a " == " #b)

class FixedWidget : public Widget {
public:
    explicit FixedWidget(Size size) : size_(size) {}
    FixedWidget(Size size, Color color, String text)
        : size_(size), color_(color), text_(std::move(text)) {}
    Size measure_impl(Size available, const PaintContext* = nullptr) override {
        return Size{size_.width < available.width ? size_.width : available.width,
                    size_.height < available.height ? size_.height : available.height};
    }

    void paint_impl(PaintContext& ctx) override {
        ctx.fill_rounded(bounds_, color_, 4.0f);
        ctx.draw_text(text_, bounds_, Color{0xFF, 0xFF, 0xFF});
    }

private:
    Size size_;
    Color color_{0, 0, 0, 0};
    String text_;
};

class AdaptiveWidget : public Widget {
public:
    explicit AdaptiveWidget(Size pref) : pref_(pref) {}
    Size measure_impl(Size available, const PaintContext* = nullptr) override {
        const f32 w = available.width > 0.0f && available.width < pref_.width
                          ? available.width
                          : pref_.width;
        const f32 h = available.height > 0.0f && available.height < pref_.height
                          ? available.height
                          : pref_.height;
        return Size{w, h};
    }

private:
    Size pref_;
};

void test_stack_vertical() {
    StackPanel panel(Orientation::Vertical);
    panel.set_padding(0.0f);
    panel.set_spacing(8.0f);

    FixedWidget a(Size{80.0f, 32.0f});
    FixedWidget b(Size{80.0f, 32.0f});
    FixedWidget c(Size{80.0f, 32.0f});
    panel.append_child(&a);
    panel.append_child(&b);
    panel.append_child(&c);

    panel.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    panel.perform_layout();

    CHECK(a.bounds().left == 0.0f);
    CHECK(b.bounds().left == 0.0f);
    CHECK(c.bounds().left == 0.0f);
    CHECK(a.bounds().top == 0.0f);
    CHECK(b.bounds().top == 32.0f + 8.0f);
    CHECK(c.bounds().top == 32.0f * 2.0f + 8.0f * 2.0f);
    CHECK(c.bounds().bottom == 32.0f * 3.0f + 8.0f * 2.0f);
}

void test_stack_horizontal() {
    StackPanel panel(Orientation::Horizontal);
    panel.set_padding(0.0f);
    panel.set_spacing(10.0f);

    FixedWidget a(Size{80.0f, 32.0f});
    FixedWidget b(Size{50.0f, 20.0f});
    FixedWidget c(Size{30.0f, 10.0f});
    panel.append_child(&a);
    panel.append_child(&b);
    panel.append_child(&c);

    const Size total = panel.measure(Size{300.0f, 60.0f});
    CHECK(total.width == 80.0f + 50.0f + 30.0f + 10.0f * 2.0f);
    CHECK(total.height == 32.0f);

    panel.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 60.0f));
    panel.perform_layout();

    CHECK(panel.first_child() == &a);
    CHECK(a.next_sibling() == &b);
    CHECK(b.next_sibling() == &c);
    CHECK(a.bounds().left == 0.0f);
    CHECK(b.bounds().left == 80.0f + 10.0f);
    CHECK(c.bounds().left == 80.0f + 10.0f + 50.0f + 10.0f);
    CHECK(a.bounds().top == 0.0f);
    CHECK(b.bounds().top == 0.0f);
    CHECK(c.bounds().top == 0.0f);
}

void test_margin() {
    StackPanel panel(Orientation::Vertical);
    panel.set_padding(0.0f);
    panel.set_spacing(0.0f);

    StackPanel child_panel(Orientation::Vertical);
    child_panel.set_padding(0.0f);
    child_panel.set_spacing(0.0f);
    child_panel.set_margin(Margins{10.0f, 10.0f, 10.0f, 10.0f});

    FixedWidget inner(Size{50.0f, 30.0f});
    child_panel.append_child(&inner);
    panel.append_child(&child_panel);

    const Size total = panel.measure(Size{200.0f, 200.0f});
    CHECK(total.width == 50.0f + 20.0f);
    CHECK(total.height == 30.0f + 20.0f);

    panel.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    panel.perform_layout();

    CHECK(child_panel.bounds().height() == 30.0f);
    CHECK(inner.bounds().left == 0.0f);
    CHECK(inner.bounds().top == 0.0f);
    CHECK(inner.bounds().height() == 30.0f);
}

void test_zero_size() {
    StackPanel p0(Orientation::Vertical);
    p0.set_padding(0.0f);
    p0.set_spacing(0.0f);
    FixedWidget z0(Size{0.0f, 0.0f});
    FixedWidget a0(Size{80.0f, 32.0f});
    FixedWidget b0(Size{80.0f, 32.0f});
    p0.append_child(&z0);
    p0.append_child(&a0);
    p0.append_child(&b0);
    p0.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    p0.perform_layout();
    CHECK(a0.bounds().top == 0.0f);
    CHECK(b0.bounds().top == 32.0f);

    StackPanel p1(Orientation::Vertical);
    p1.set_padding(0.0f);
    p1.set_spacing(8.0f);
    FixedWidget z1(Size{0.0f, 0.0f});
    FixedWidget a1(Size{80.0f, 32.0f});
    FixedWidget b1(Size{80.0f, 32.0f});
    p1.append_child(&a1);
    p1.append_child(&z1);
    p1.append_child(&b1);
    p1.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    p1.perform_layout();
    CHECK(a1.bounds().top == 0.0f);
    CHECK(b1.bounds().top == 32.0f + 8.0f);
    CHECK(p1.measure(Size{200.0f, 200.0f}).height == 32.0f * 2.0f + 8.0f);
}

void test_available_size() {
    StackPanel v1(Orientation::Vertical);
    v1.set_padding(0.0f);
    v1.set_spacing(0.0f);
    FixedWidget fa(Size{120.0f, 32.0f});
    v1.append_child(&fa);
    const Size d1 = v1.measure(Size{30.0f, 100.0f});
    CHECK(d1.width == 30.0f);
    CHECK(d1.height == 32.0f);

    StackPanel v2(Orientation::Vertical);
    v2.set_padding(0.0f);
    v2.set_spacing(0.0f);
    AdaptiveWidget aa(Size{120.0f, 32.0f});
    v2.append_child(&aa);
    const Size d2 = v2.measure(Size{50.0f, 100.0f});
    CHECK(d2.width == 50.0f);
    CHECK(d2.height == 32.0f);

    StackPanel v3(Orientation::Vertical);
    v3.set_padding(0.0f);
    v3.set_spacing(0.0f);
    FixedWidget fa2(Size{80.0f, 32.0f});
    v3.append_child(&fa2);
    const Size d3 = v3.measure(Size{0.0f, 0.0f});
    CHECK(d3.width == 0.0f);
    CHECK(d3.height == 0.0f);
}

void test_wrap_panel_wrap() {
    WrapPanel wrap;
    wrap.set_spacing(10.0f);
    wrap.set_line_spacing(10.0f);

    FixedWidget a(Size{50.0f, 30.0f});
    FixedWidget b(Size{50.0f, 30.0f});
    FixedWidget c(Size{50.0f, 30.0f});
    FixedWidget d(Size{50.0f, 30.0f});
    FixedWidget e(Size{50.0f, 30.0f});
    FixedWidget f(Size{50.0f, 30.0f});
    wrap.append_child(&a);
    wrap.append_child(&b);
    wrap.append_child(&c);
    wrap.append_child(&d);
    wrap.append_child(&e);
    wrap.append_child(&f);

    // 170px wide: 50+10+50+10+50 = 170 → exactly 3 per row, 2 rows
    const Size m = wrap.measure(Size{170.0f, 200.0f});
    CHECK(m.width == 50.0f * 3.0f + 10.0f * 2.0f);
    CHECK(m.height == 30.0f * 2.0f + 10.0f);

    wrap.set_bounds(RectF::make(0.0f, 0.0f, 170.0f, 200.0f));
    wrap.perform_layout();

    // Row 1
    CHECK(a.bounds().left == 0.0f && a.bounds().top == 0.0f);
    CHECK(b.bounds().left == 50.0f + 10.0f && b.bounds().top == 0.0f);
    CHECK(c.bounds().left == 50.0f * 2.0f + 10.0f * 2.0f && c.bounds().top == 0.0f);

    // Row 2 (wrapped)
    CHECK(d.bounds().left == 0.0f && d.bounds().top == 30.0f + 10.0f);
    CHECK(e.bounds().left == 50.0f + 10.0f && e.bounds().top == 30.0f + 10.0f);
    CHECK(f.bounds().left == 50.0f * 2.0f + 10.0f * 2.0f && f.bounds().top == 30.0f + 10.0f);
}

void test_wrap_panel_gap() {
    WrapPanel wrap;
    wrap.set_spacing(10.0f);
    wrap.set_line_spacing(10.0f);

    FixedWidget a(Size{50.0f, 30.0f});
    FixedWidget b(Size{50.0f, 30.0f});
    FixedWidget c(Size{50.0f, 30.0f});
    FixedWidget d(Size{50.0f, 30.0f});
    FixedWidget e(Size{50.0f, 30.0f});
    wrap.append_child(&a);
    wrap.append_child(&b);
    wrap.append_child(&c);
    wrap.append_child(&d);
    wrap.append_child(&e);

    // 120px wide: 50+10+50 = 110 fits two, the third wraps
    wrap.set_bounds(RectF::make(0.0f, 0.0f, 120.0f, 200.0f));
    wrap.perform_layout();

    // Row 1 gap baseline
    CHECK(a.bounds().left == 0.0f);
    CHECK(b.bounds().left == 50.0f + 10.0f);  // width1 + gap
    // c wraps to row 2: X resets to 0, no gap
    CHECK(c.bounds().top == 30.0f + 10.0f);
    CHECK(c.bounds().left == 0.0f);
    // Row 2 uses the same gap
    CHECK(d.bounds().left == 50.0f + 10.0f);
    // e wraps to row 3: X resets to 0
    CHECK(e.bounds().top == 30.0f * 2.0f + 10.0f * 2.0f);
    CHECK(e.bounds().left == 0.0f);
}

void test_wrap_panel_demo() {
    WrapPanel wrap;
    wrap.set_spacing(8.0f);
    wrap.set_line_spacing(8.0f);

    FixedWidget items[14] = {FixedWidget(Size{90.0f, 28.0f}), FixedWidget(Size{90.0f, 28.0f}),
                             FixedWidget(Size{90.0f, 28.0f}), FixedWidget(Size{90.0f, 28.0f}),
                             FixedWidget(Size{90.0f, 28.0f}), FixedWidget(Size{90.0f, 28.0f}),
                             FixedWidget(Size{90.0f, 28.0f}), FixedWidget(Size{90.0f, 28.0f}),
                             FixedWidget(Size{90.0f, 28.0f}), FixedWidget(Size{90.0f, 28.0f}),
                             FixedWidget(Size{90.0f, 28.0f}), FixedWidget(Size{90.0f, 28.0f}),
                             FixedWidget(Size{90.0f, 28.0f}), FixedWidget(Size{90.0f, 28.0f})};
    for (int i = 0; i < 14; ++i) wrap.append_child(&items[i]);

    // Nested demo scenario: wrap is a child of a vertical StackPanel, stretched to 1024 wide
    StackPanel host(Orientation::Vertical);
    host.set_padding(0.0f);
    host.set_spacing(0.0f);
    host.append_child(&wrap);
    host.set_bounds(RectF::make(0.0f, 0.0f, 1024.0f, 720.0f));
    host.perform_layout();

    // Row 1 last item (item 10): x = 9 * 98 = 882
    CHECK(items[9].bounds().left == 9.0f * 98.0f);
    CHECK(items[9].bounds().top == 0.0f);
    // item 11 wraps to row 2: X must reset to 0
    CHECK(items[10].bounds().left == 0.0f);
    CHECK(items[10].bounds().top == 28.0f + 8.0f);
    // item 13 is the 3rd in row 2
    CHECK(items[12].bounds().left == 98.0f * 2.0f);
    CHECK(items[12].bounds().top == 28.0f + 8.0f);
}

void test_wrap_boundary() {
    // Exactly fills one row: 50+10+50 = 110, container 110 → 2 fit exactly, the 3rd wraps
    WrapPanel wa;
    wa.set_spacing(10.0f);
    wa.set_line_spacing(10.0f);
    FixedWidget a1(Size{50.0f, 30.0f});
    FixedWidget a2(Size{50.0f, 30.0f});
    FixedWidget a3(Size{50.0f, 30.0f});
    wa.append_child(&a1);
    wa.append_child(&a2);
    wa.append_child(&a3);
    wa.set_bounds(RectF::make(0.0f, 0.0f, 110.0f, 200.0f));
    wa.perform_layout();
    CHECK(a1.bounds().left == 0.0f && a1.bounds().top == 0.0f);
    CHECK(a2.bounds().left == 60.0f && a2.bounds().top == 0.0f);
    CHECK(a2.bounds().right == 110.0f);  // Fills the row exactly, no wrap
    CHECK(a3.bounds().left == 0.0f && a3.bounds().top == 40.0f);  // Wrapped

    // Barely overflows: 110 > 109 → the 2nd item already wraps
    WrapPanel wb;
    wb.set_spacing(10.0f);
    wb.set_line_spacing(10.0f);
    FixedWidget b1(Size{50.0f, 30.0f});
    FixedWidget b2(Size{50.0f, 30.0f});
    FixedWidget b3(Size{50.0f, 30.0f});
    wb.append_child(&b1);
    wb.append_child(&b2);
    wb.append_child(&b3);
    wb.set_bounds(RectF::make(0.0f, 0.0f, 109.0f, 200.0f));
    wb.perform_layout();
    CHECK(b1.bounds().left == 0.0f && b1.bounds().top == 0.0f);
    CHECK(b2.bounds().left == 0.0f && b2.bounds().top == 40.0f);  // Wraps on overflow
    CHECK(b3.bounds().left == 0.0f && b3.bounds().top == 80.0f);
}

void test_wrap_single() {
    WrapPanel wrap;
    wrap.set_spacing(10.0f);
    wrap.set_line_spacing(10.0f);
    FixedWidget a(Size{50.0f, 30.0f});
    wrap.append_child(&a);

    // measure does not add a trailing gap
    const Size m = wrap.measure(Size{200.0f, 200.0f});
    CHECK(m.width == 50.0f);
    CHECK(m.height == 30.0f);

    wrap.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    wrap.perform_layout();
    CHECK(a.bounds().left == 0.0f && a.bounds().top == 0.0f);
    CHECK(a.bounds().width() == 50.0f && a.bounds().height() == 30.0f);
}

void test_wrap_measure_arrange_consistency() {
    const f32 kWidths[] = {60.0f, 40.0f, 80.0f, 30.0f, 50.0f, 70.0f};
    const f32 kHeights[] = {20.0f, 30.0f, 25.0f, 40.0f, 20.0f, 35.0f};
    const f32 kAvailW = 150.0f;

    WrapPanel wrap;
    wrap.set_spacing(10.0f);
    wrap.set_line_spacing(10.0f);
    FixedWidget items[6] = {FixedWidget(Size{kWidths[0], kHeights[0]}),
                            FixedWidget(Size{kWidths[1], kHeights[1]}),
                            FixedWidget(Size{kWidths[2], kHeights[2]}),
                            FixedWidget(Size{kWidths[3], kHeights[3]}),
                            FixedWidget(Size{kWidths[4], kHeights[4]}),
                            FixedWidget(Size{kWidths[5], kHeights[5]})};
    for (int i = 0; i < 6; ++i) wrap.append_child(&items[i]);

    // Independently predict each item's coordinates using measure's wrap logic (x includes intra-row gap)
    f32 pred_x[6] = {};
    f32 pred_y[6] = {};
    f32 line_w = 0.0f;
    f32 line_h = 0.0f;
    f32 y = 0.0f;
    for (int i = 0; i < 6; ++i) {
        f32 add = (line_w > 0.0f ? 10.0f : 0.0f) + kWidths[i];
        if (line_w > 0.0f && line_w + add > kAvailW) {
            y += line_h + 10.0f;
            line_w = 0.0f;
            line_h = 0.0f;
            add = kWidths[i];
        }
        pred_x[i] = line_w + (line_w > 0.0f ? 10.0f : 0.0f);
        pred_y[i] = y;
        line_w += add;
        line_h = line_h > kHeights[i] ? line_h : kHeights[i];
    }

    const Size m = wrap.measure(Size{kAvailW, 200.0f});
    wrap.set_bounds(RectF::make(0.0f, 0.0f, kAvailW, 200.0f));
    wrap.perform_layout();

    // Measure-predicted coordinates must exactly match Arrange placement
    for (int i = 0; i < 6; ++i) {
        CHECK(items[i].bounds().left == pred_x[i]);
        CHECK(items[i].bounds().top == pred_y[i]);
    }
    // Measure total matches the actual Arrange footprint (max row width)
    CHECK(m.width == 130.0f);
    CHECK(m.height == pred_y[5] + kHeights[5]);
}

void test_grid_fixed() {
    // Fixed 3×2 grid, all Auto columns/rows (content-sized)
    GridPanel grid(3, 2);
    grid.set_gap(8.0f);
    FixedWidget a(Size{50.0f, 30.0f});
    FixedWidget b(Size{60.0f, 20.0f});
    FixedWidget c(Size{40.0f, 40.0f});
    FixedWidget d(Size{30.0f, 50.0f});
    FixedWidget e(Size{70.0f, 25.0f});
    FixedWidget f(Size{55.0f, 35.0f});
    grid.add(&a, 0, 0);
    grid.add(&b, 1, 0);
    grid.add(&c, 2, 0);
    grid.add(&d, 0, 1);
    grid.add(&e, 1, 1);
    grid.add(&f, 2, 1);

    grid.set_bounds(RectF::make(0.0f, 0.0f, 400.0f, 300.0f));
    grid.perform_layout();

    // Column widths: col0=max(50,30)=50, col1=max(60,70)=70, col2=max(40,55)=55
    // Row heights: row0=max(30,20,40)=40, row1=max(50,25,35)=50
    CHECK(a.bounds().left == 0.0f && a.bounds().top == 0.0f);
    CHECK(a.bounds().width() == 50.0f && a.bounds().height() == 40.0f);
    CHECK(b.bounds().left == 58.0f && b.bounds().top == 0.0f);
    CHECK(b.bounds().width() == 70.0f && b.bounds().height() == 40.0f);
    CHECK(c.bounds().left == 136.0f && c.bounds().top == 0.0f);
    CHECK(c.bounds().width() == 55.0f && c.bounds().height() == 40.0f);
    CHECK(d.bounds().left == 0.0f && d.bounds().top == 48.0f);
    CHECK(d.bounds().width() == 50.0f && d.bounds().height() == 50.0f);
    CHECK(e.bounds().left == 58.0f && e.bounds().top == 48.0f);
    CHECK(e.bounds().width() == 70.0f && e.bounds().height() == 50.0f);
    CHECK(f.bounds().left == 136.0f && f.bounds().top == 48.0f);
    CHECK(f.bounds().width() == 55.0f && f.bounds().height() == 50.0f);
}

void test_grid_star_columns() {
    // Auto column + star columns: col0 auto(50), col1 star(1), col2 star(2)
    GridPanel grid(3, 1);
    grid.set_gap(0.0f);
    grid.set_column_auto(0);
    grid.set_column_star(1, 1.0f);
    grid.set_column_star(2, 2.0f);
    FixedWidget a(Size{50.0f, 30.0f});
    FixedWidget b(Size{10.0f, 10.0f});
    FixedWidget c(Size{20.0f, 10.0f});
    grid.add(&a, 0, 0);
    grid.add(&b, 1, 0);
    grid.add(&c, 2, 0);

    grid.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 50.0f));
    grid.perform_layout();

    // Remaining 300-50 = 250 split 1:2 → col1=83.33, col2=166.67
    CHECK_NEAR(a.bounds().left, 0.0f);
    CHECK_NEAR(a.bounds().width(), 50.0f);
    CHECK_NEAR(b.bounds().left, 50.0f);
    CHECK_NEAR(b.bounds().width(), 250.0f / 3.0f);
    CHECK_NEAR(c.bounds().left, 50.0f + 250.0f / 3.0f);
    CHECK_NEAR(c.bounds().width(), 500.0f / 3.0f);
    CHECK_NEAR(c.bounds().right, 300.0f);
}

void test_grid_span() {
    // Spans: A at (0,0) 2×2, B(2,0), C(2,1), D(0,2) spanning 3 columns, E(1,2)
    GridPanel grid(3, 3);
    grid.set_gap(0.0f);
    FixedWidget a(Size{60.0f, 50.0f});
    FixedWidget b(Size{20.0f, 10.0f});
    FixedWidget c(Size{30.0f, 20.0f});
    FixedWidget d(Size{40.0f, 40.0f});
    FixedWidget e(Size{10.0f, 10.0f});
    grid.add(&a, 0, 0, 2, 2);
    grid.add(&b, 2, 0);
    grid.add(&c, 2, 1);
    grid.add(&d, 0, 2, 3, 1);
    grid.add(&e, 1, 2);

    grid.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    grid.perform_layout();

    // Column widths: col0=max(60/2=30, 40/3≈13.3)=30, col1=max(60/2=30, 10)=30, col2=max(20,30,13.3)=30
    // Row heights: row0=max(50/2=25, 10)=25, row1=max(50/2=25, 20)=25, row2=max(40,10)=40
    CHECK(a.bounds().left == 0.0f && a.bounds().top == 0.0f);
    CHECK(a.bounds().width() == 60.0f && a.bounds().height() == 50.0f);
    CHECK(b.bounds().left == 60.0f && b.bounds().top == 0.0f);
    CHECK(b.bounds().width() == 30.0f && b.bounds().height() == 25.0f);
    CHECK(c.bounds().left == 60.0f && c.bounds().top == 25.0f);
    CHECK(c.bounds().width() == 30.0f && c.bounds().height() == 25.0f);
    CHECK(d.bounds().left == 0.0f && d.bounds().top == 50.0f);
    CHECK(d.bounds().width() == 90.0f && d.bounds().height() == 40.0f);
    CHECK(e.bounds().left == 30.0f && e.bounds().top == 50.0f);
    CHECK(e.bounds().width() == 30.0f && e.bounds().height() == 40.0f);
}

void test_grid_star_rows() {
    // Auto row + star row: row0 auto(40), row1 star(1)
    GridPanel grid(1, 2);
    grid.set_gap(0.0f);
    grid.set_row_auto(0);
    grid.set_row_star(1, 1.0f);
    FixedWidget a(Size{50.0f, 40.0f});
    FixedWidget b(Size{10.0f, 10.0f});
    grid.add(&a, 0, 0);
    grid.add(&b, 0, 1);

    grid.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    grid.perform_layout();

    // row0 = 40 (content height); the remaining 60 goes to the star row
    CHECK(a.bounds().top == 0.0f && a.bounds().height() == 40.0f);
    CHECK(b.bounds().top == 40.0f && b.bounds().height() == 60.0f);
    CHECK(b.bounds().bottom == 100.0f);
}

void test_grid_star_rows_ratio() {
    // Both rows star: container height 300 split 1:2
    GridPanel grid(1, 2);
    grid.set_gap(0.0f);
    grid.set_row_star(0, 1.0f);
    grid.set_row_star(1, 2.0f);
    FixedWidget a(Size{10.0f, 10.0f});
    FixedWidget b(Size{10.0f, 10.0f});
    grid.add(&a, 0, 0);
    grid.add(&b, 0, 1);

    grid.set_bounds(RectF::make(0.0f, 0.0f, 100.0f, 300.0f));
    grid.perform_layout();

    // content+share: remaining 280 split 1:2 → row0=10+280/3, row1=10+560/3
    CHECK_NEAR(a.bounds().height(), 10.0f + 280.0f / 3.0f);
    CHECK_NEAR(b.bounds().top, 10.0f + 280.0f / 3.0f);
    CHECK_NEAR(b.bounds().height(), 10.0f + 560.0f / 3.0f);
    CHECK_NEAR(b.bounds().bottom, 300.0f);
}

void test_grid_star_rows_gap() {
    // Auto row + two star rows, gap 10, container height 150
    GridPanel grid(1, 3);
    grid.set_gap(10.0f);
    grid.set_row_star(1, 1.0f);
    grid.set_row_star(2, 1.0f);
    FixedWidget a(Size{50.0f, 40.0f});
    FixedWidget b(Size{10.0f, 10.0f});
    FixedWidget c(Size{10.0f, 10.0f});
    grid.add(&a, 0, 0);
    grid.add(&b, 0, 1);
    grid.add(&c, 0, 2);

    grid.set_bounds(RectF::make(0.0f, 0.0f, 100.0f, 150.0f));
    grid.perform_layout();

    // Remaining = 150 - 40 - 20 (gap) = 90 → 45 per star row
    CHECK(a.bounds().top == 0.0f && a.bounds().height() == 40.0f);
    CHECK_NEAR(b.bounds().top, 50.0f);
    CHECK_NEAR(b.bounds().height(), 45.0f);
    CHECK_NEAR(c.bounds().top, 105.0f);
    CHECK_NEAR(c.bounds().height(), 45.0f);
    CHECK_NEAR(c.bounds().bottom, 150.0f);
}

void test_grid_star_rows_span() {
    // Spanning two star rows: A at (0,0) 1×2; B/C sit in the star rows
    GridPanel grid(2, 3);
    grid.set_gap(0.0f);
    grid.set_row_star(1, 1.0f);
    grid.set_row_star(2, 1.0f);
    FixedWidget a(Size{50.0f, 40.0f});
    FixedWidget b(Size{10.0f, 10.0f});
    FixedWidget c(Size{10.0f, 10.0f});
    grid.add(&a, 0, 0, 1, 2);
    grid.add(&b, 1, 1);
    grid.add(&c, 1, 2);

    grid.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    grid.perform_layout();

    // content: row0=40/2=20, row1=max(20,10)=20, row2=10 → used 50, 150 remaining
    // Each star row gets +75 → row_h = [20, 95, 85]
    CHECK(a.bounds().top == 0.0f);
    CHECK(a.bounds().height() == 20.0f + 95.0f);
    CHECK(b.bounds().top == 20.0f && b.bounds().height() == 95.0f);
    CHECK(c.bounds().top == 115.0f && c.bounds().height() == 85.0f);
}

void test_grid_star_fill() {
    // Constrained available: the star grid fills the desired size
    GridPanel grid(2, 1);
    grid.set_gap(0.0f);
    grid.set_column_star(0, 1.0f);
    grid.set_column_star(1, 2.0f);
    FixedWidget a(Size{30.0f, 20.0f});
    FixedWidget b(Size{40.0f, 20.0f});
    grid.add(&a, 0, 0);
    grid.add(&b, 1, 0);
    Size s = grid.measure(Size{300.0f, 50.0f});
    CHECK_NEAR(s.width, 300.0f);
    CHECK_NEAR(s.height, 20.0f);

    // Unlimited available: falls back to the content size 70
    Size s2 = grid.measure(Size{1e7f, 1e7f});
    CHECK_NEAR(s2.width, 70.0f);
    CHECK_NEAR(s2.height, 20.0f);

    // Row stars likewise fill the height
    GridPanel grid2(1, 2);
    grid2.set_gap(0.0f);
    grid2.set_row_star(0, 1.0f);
    grid2.set_row_star(1, 1.0f);
    FixedWidget c(Size{50.0f, 30.0f});
    FixedWidget d(Size{10.0f, 10.0f});
    grid2.add(&c, 0, 0);
    grid2.add(&d, 0, 1);
    Size s3 = grid2.measure(Size{100.0f, 200.0f});
    CHECK_NEAR(s3.width, 50.0f);
    CHECK_NEAR(s3.height, 200.0f);

    // No star: unaffected by available
    GridPanel plain(2, 1);
    plain.set_gap(0.0f);
    plain.add(&a, 0, 0);
    plain.add(&b, 1, 0);
    Size s4 = plain.measure(Size{300.0f, 50.0f});
    CHECK_NEAR(s4.width, 70.0f);
}

void test_grid_star_fill_stack() {
    // Star grid in a vertical StackPanel fills the width; columns split 1:2:1
    StackPanel panel;
    panel.set_bounds(RectF::make(0.0f, 0.0f, 400.0f, 200.0f));
    GridPanel grid(3, 1);
    grid.set_gap(0.0f);
    grid.set_column_star(0, 1.0f);
    grid.set_column_star(1, 2.0f);
    grid.set_column_star(2, 1.0f);
    FixedWidget a(Size{30.0f, 40.0f});
    FixedWidget b(Size{30.0f, 40.0f});
    FixedWidget c(Size{30.0f, 40.0f});
    grid.add(&a, 0, 0);
    grid.add(&b, 1, 0);
    grid.add(&c, 2, 0);
    panel.append_child(&grid);
    panel.perform_layout();

    // Grid desired width = 400 (filled); columns content+share: each 30 + 310/4
    CHECK_NEAR(grid.desired_size().width, 400.0f);
    CHECK_NEAR(grid.width(), 400.0f);
    CHECK_NEAR(grid.height(), 40.0f);
    const f32 share = 310.0f / 4.0f;
    CHECK_NEAR(a.width(), 30.0f + share);
    CHECK_NEAR(b.width(), 30.0f + 2.0f * share);
    CHECK_NEAR(c.width(), 30.0f + share);
    CHECK_NEAR(c.bounds().right, 400.0f);
}

void test_measure_min_max() {
    // min clamp: 50x40 + min 80x60 → 80x60
    FixedWidget a(Size{50.0f, 40.0f});
    a.set_min_size(Size{80.0f, 60.0f});
    Size s = a.measure(Size{1000.0f, 1000.0f});
    CHECK_NEAR(s.width, 80.0f);
    CHECK_NEAR(s.height, 60.0f);

    // max clamp: 200x100 + max 60x30 → 60x30
    FixedWidget b(Size{200.0f, 100.0f});
    b.set_max_size(Size{60.0f, 30.0f});
    Size s2 = b.measure(Size{1000.0f, 1000.0f});
    CHECK_NEAR(s2.width, 60.0f);
    CHECK_NEAR(s2.height, 30.0f);

    // Within min/max range: keeps its own size
    FixedWidget c(Size{50.0f, 50.0f});
    c.set_min_size(Size{30.0f, 30.0f});
    c.set_max_size(Size{200.0f, 200.0f});
    Size s3 = c.measure(Size{1e7f, 1e7f});
    CHECK_NEAR(s3.width, 50.0f);
    CHECK_NEAR(s3.height, 50.0f);

    // Effective in layout: in a vertical stack, 50x40 + min 80x60 → height 60
    StackPanel panel;
    panel.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 200.0f));
    FixedWidget d(Size{50.0f, 40.0f});
    d.set_min_size(Size{80.0f, 60.0f});
    panel.append_child(&d);
    panel.perform_layout();
    CHECK_NEAR(d.desired_size().width, 80.0f);
    CHECK_NEAR(d.height(), 60.0f);

    // max effective in layout: 200x100 + max 60x30 → height 30
    FixedWidget e(Size{200.0f, 100.0f});
    e.set_max_size(Size{60.0f, 30.0f});
    panel.append_child(&e);
    panel.perform_layout();
    CHECK_NEAR(e.desired_size().width, 60.0f);
    CHECK_NEAR(e.height(), 30.0f);
}

void test_dock_basic() {
    // Four directions + Fill, container 300×200
    DockPanel panel;
    FixedWidget left(Size{60.0f, 50.0f});
    FixedWidget top(Size{40.0f, 60.0f});
    FixedWidget right(Size{50.0f, 30.0f});
    FixedWidget bottom(Size{30.0f, 40.0f});
    FixedWidget fill(Size{10.0f, 10.0f});
    panel.dock(&left, Dock::Left);
    panel.dock(&right, Dock::Right);
    panel.dock(&top, Dock::Top);
    panel.dock(&bottom, Dock::Bottom);
    panel.dock(&fill, Dock::Fill);

    panel.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 200.0f));
    panel.perform_layout();

    CHECK(left.bounds().left == 0.0f && left.bounds().top == 0.0f);
    CHECK(left.bounds().width() == 60.0f && left.bounds().height() == 200.0f);
    // right is the 2nd dock: width 50, height = remaining 200, x = 300-50 = 250
    CHECK(right.bounds().left == 250.0f && right.bounds().top == 0.0f);
    CHECK(right.bounds().width() == 50.0f && right.bounds().height() == 200.0f);
    // top is the 3rd: width = 300-60-50 = 190, height = child's 60
    CHECK(top.bounds().left == 60.0f && top.bounds().top == 0.0f);
    CHECK(top.bounds().width() == 190.0f && top.bounds().height() == 60.0f);
    // bottom is the 4th: width 190, height 40, y = 200-40 = 160
    CHECK(bottom.bounds().left == 60.0f && bottom.bounds().top == 160.0f);
    CHECK(bottom.bounds().width() == 190.0f && bottom.bounds().height() == 40.0f);
    // fill gets the rest: x=60, y=60, w=190, h=160-60=100
    CHECK(fill.bounds().left == 60.0f && fill.bounds().top == 60.0f);
    CHECK(fill.bounds().width() == 190.0f && fill.bounds().height() == 100.0f);
}

void test_dock_stacking() {
    // Stacked docks: Left+Left+Top+Right → Fill takes the rest
    DockPanel panel;
    FixedWidget l1(Size{80.0f, 40.0f});
    FixedWidget l2(Size{70.0f, 30.0f});
    FixedWidget t(Size{50.0f, 60.0f});
    FixedWidget r(Size{90.0f, 20.0f});
    FixedWidget fill(Size{10.0f, 10.0f});
    panel.dock(&l1, Dock::Left);
    panel.dock(&l2, Dock::Left);
    panel.dock(&t, Dock::Top);
    panel.dock(&r, Dock::Right);
    panel.dock(&fill, Dock::Fill);

    // measure: width = 80+70+90 = 240, height = 60
    const Size m = panel.measure(Size{400.0f, 300.0f});
    CHECK(m.width == 240.0f);
    CHECK(m.height == 60.0f);

    panel.set_bounds(RectF::make(0.0f, 0.0f, 400.0f, 300.0f));
    panel.perform_layout();

    CHECK(l1.bounds().left == 0.0f && l1.bounds().width() == 80.0f);
    CHECK(l1.bounds().height() == 300.0f);
    CHECK(l2.bounds().left == 80.0f && l2.bounds().width() == 70.0f);
    // top after l1+l2: x=150, width = 400-150 = 250, height = child's 60
    CHECK(t.bounds().left == 150.0f && t.bounds().top == 0.0f);
    CHECK(t.bounds().width() == 250.0f && t.bounds().height() == 60.0f);
    // right: x = 400-90 = 310, y = 60, height = 300-60 = 240
    CHECK(r.bounds().left == 310.0f && r.bounds().top == 60.0f);
    CHECK(r.bounds().width() == 90.0f && r.bounds().height() == 240.0f);
    // fill rest: x=150, y=60, w=160, h=240
    CHECK(fill.bounds().left == 150.0f && fill.bounds().top == 60.0f);
    CHECK(fill.bounds().width() == 160.0f && fill.bounds().height() == 240.0f);
}

void test_dock_gap() {
    // DockPanel with gap: 10, four directions + Fill
    DockPanel panel;
    panel.set_gap(10.0f);
    FixedWidget left(Size{60.0f, 40.0f});
    FixedWidget top(Size{40.0f, 50.0f});
    FixedWidget right(Size{50.0f, 30.0f});
    FixedWidget bottom(Size{30.0f, 40.0f});
    FixedWidget fill(Size{10.0f, 10.0f});
    panel.dock(&left, Dock::Left);
    panel.dock(&top, Dock::Top);
    panel.dock(&right, Dock::Right);
    panel.dock(&bottom, Dock::Bottom);
    panel.dock(&fill, Dock::Fill);

    // measure: w = 60+10+50+10 = 130, h = 50+10+40+10 = 110
    const Size m = panel.measure(Size{300.0f, 200.0f});
    CHECK(m.width == 130.0f);
    CHECK(m.height == 110.0f);

    panel.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 200.0f));
    panel.perform_layout();

    CHECK(left.bounds().left == 0.0f && left.bounds().top == 0.0f);
    CHECK(left.bounds().width() == 60.0f && left.bounds().height() == 200.0f);
    // top between left and right: x = 60+10 = 70, width = 300-70 = 230, height = child's 50
    CHECK(top.bounds().left == 70.0f && top.bounds().top == 0.0f);
    CHECK(top.bounds().width() == 230.0f && top.bounds().height() == 50.0f);
    // right: x = 300-50 = 250, y = 50+10 = 60, height = 200-60 = 140
    CHECK(right.bounds().left == 250.0f && right.bounds().top == 60.0f);
    CHECK(right.bounds().width() == 50.0f && right.bounds().height() == 140.0f);
    // bottom: y = 200-40 = 160, width = 240-70 = 170
    CHECK(bottom.bounds().left == 70.0f && bottom.bounds().top == 160.0f);
    CHECK(bottom.bounds().width() == 170.0f && bottom.bounds().height() == 40.0f);
    // fill rest
    CHECK(fill.bounds().left == 70.0f && fill.bounds().top == 60.0f);
    CHECK(fill.bounds().width() == 170.0f && fill.bounds().height() == 90.0f);
}

// General layout invariant: arrange rects are local to the parent panel; global
// position = parent's + child's. Global coords in arrange would misalign local
// hit_test queries → assertion failure.
void verify_local_coords(Widget* panel, Widget* child, f32 parent_x, f32 parent_y) {
    CHECK(child->bounds().left >= 0.0f && child->bounds().top >= 0.0f);
    CHECK(child->bounds().left + child->bounds().width() <= panel->bounds().width());
    CHECK(child->bounds().top + child->bounds().height() <= panel->bounds().height());
    // A child's local coords (inside the parent) must hit: the system accumulates parent rect + child rect
    const f32 gx = parent_x + child->bounds().left + 1.0f;
    const f32 gy = parent_y + child->bounds().top + 1.0f;
    CHECK(panel->hit_test(gx - parent_x, gy - parent_y) == child);
}

void test_invariant_local_coords() {
    const f32 px = 50.0f;
    const f32 py = 30.0f;

    // StackPanel
    StackPanel sp(Orientation::Vertical);
    sp.set_padding(0.0f);
    sp.set_spacing(0.0f);
    FixedWidget s1(Size{40.0f, 30.0f});
    FixedWidget s2(Size{40.0f, 30.0f});
    sp.append_child(&s1);
    sp.append_child(&s2);
    sp.set_bounds(RectF::make(px, py, 200.0f, 160.0f));
    sp.perform_layout();
    CHECK(s1.bounds().left == 0.0f && s1.bounds().top == 0.0f);
    CHECK(s2.bounds().left == 0.0f && s2.bounds().top == 30.0f);
    verify_local_coords(&sp, &s1, px, py);
    verify_local_coords(&sp, &s2, px, py);

    // WrapPanel (wrap resets to 0, still local)
    WrapPanel wp;
    wp.set_spacing(0.0f);
    wp.set_line_spacing(0.0f);
    FixedWidget w1(Size{60.0f, 40.0f});
    FixedWidget w2(Size{60.0f, 40.0f});
    FixedWidget w3(Size{60.0f, 40.0f});
    wp.append_child(&w1);
    wp.append_child(&w2);
    wp.append_child(&w3);
    wp.set_bounds(RectF::make(px, py, 120.0f, 160.0f));
    wp.perform_layout();
    CHECK(w1.bounds().left == 0.0f && w1.bounds().top == 0.0f);
    CHECK(w2.bounds().left == 60.0f && w2.bounds().top == 0.0f);
    CHECK(w3.bounds().left == 0.0f && w3.bounds().top == 40.0f);
    verify_local_coords(&wp, &w1, px, py);
    verify_local_coords(&wp, &w2, px, py);
    verify_local_coords(&wp, &w3, px, py);

    // GridPanel
    GridPanel gp(2, 1);
    gp.set_gap(0.0f);
    FixedWidget g1(Size{50.0f, 30.0f});
    FixedWidget g2(Size{60.0f, 30.0f});
    gp.add(&g1, 0, 0);
    gp.add(&g2, 1, 0);
    gp.set_bounds(RectF::make(px, py, 200.0f, 100.0f));
    gp.perform_layout();
    CHECK(g1.bounds().left == 0.0f && g1.bounds().top == 0.0f);
    CHECK(g2.bounds().left == 50.0f && g2.bounds().top == 0.0f);
    verify_local_coords(&gp, &g1, px, py);
    verify_local_coords(&gp, &g2, px, py);

    // DockPanel
    DockPanel dp;
    FixedWidget d1(Size{60.0f, 40.0f});
    FixedWidget d2(Size{40.0f, 40.0f});
    dp.dock(&d1, Dock::Left);
    dp.dock(&d2, Dock::Fill);
    dp.set_bounds(RectF::make(px, py, 200.0f, 100.0f));
    dp.perform_layout();
    CHECK(d1.bounds().left == 0.0f && d1.bounds().top == 0.0f);
    CHECK(d2.bounds().left == 60.0f && d2.bounds().top == 0.0f);
    verify_local_coords(&dp, &d1, px, py);
    verify_local_coords(&dp, &d2, px, py);
}

// General layout invariant: a child's DesiredSize must not exceed the availableSize it was
// measured with; panels obey this too when measured by their own parent.
void test_invariant_desired_size() {
    // 1. Leaf level: fixed size constrained by available
    FixedWidget f(Size{120.0f, 32.0f});
    const Size sf = f.measure(Size{30.0f, 20.0f});
    CHECK(sf.width == 30.0f && sf.height == 20.0f);

    // 2. StackPanel: neither children nor panel exceed available
    StackPanel sp(Orientation::Vertical);
    sp.set_padding(0.0f);
    sp.set_spacing(0.0f);
    FixedWidget s1(Size{120.0f, 32.0f});
    FixedWidget s2(Size{40.0f, 60.0f});
    sp.append_child(&s1);
    sp.append_child(&s2);
    const Size ds = sp.measure(Size{30.0f, 50.0f});
    CHECK(ds.width <= 30.0f && ds.height <= 50.0f);

    // 3. WrapPanel: children + panel
    WrapPanel wp;
    wp.set_spacing(0.0f);
    wp.set_line_spacing(0.0f);
    FixedWidget w1(Size{80.0f, 40.0f});
    FixedWidget w2(Size{80.0f, 40.0f});
    wp.append_child(&w1);
    wp.append_child(&w2);
    const Size dw = wp.measure(Size{60.0f, 30.0f});
    CHECK(dw.width <= 60.0f && dw.height <= 30.0f);

    // 4. GridPanel: children + panel
    GridPanel gp(2, 1);
    gp.set_gap(0.0f);
    FixedWidget g1(Size{50.0f, 30.0f});
    FixedWidget g2(Size{60.0f, 30.0f});
    gp.add(&g1, 0, 0);
    gp.add(&g2, 1, 0);
    const Size dg = gp.measure(Size{40.0f, 40.0f});
    CHECK(dg.width <= 40.0f && dg.height <= 40.0f);

    // 5. DockPanel: children + panel
    DockPanel dp;
    FixedWidget d1(Size{60.0f, 40.0f});
    FixedWidget d2(Size{40.0f, 40.0f});
    dp.dock(&d1, Dock::Left);
    dp.dock(&d2, Dock::Fill);
    const Size dd = dp.measure(Size{40.0f, 40.0f});
    CHECK(dd.width <= 40.0f && dd.height <= 40.0f);

    // 6. Layout consistency: with constrained available, child bounds stay inside the panel
    StackPanel sp2(Orientation::Vertical);
    sp2.set_padding(0.0f);
    sp2.set_spacing(0.0f);
    FixedWidget s3(Size{120.0f, 32.0f});
    sp2.append_child(&s3);
    sp2.set_bounds(RectF::make(0.0f, 0.0f, 30.0f, 100.0f));
    sp2.perform_layout();
    CHECK(s3.bounds().width() <= 30.0f);
}

// General layout invariant: the layout box extends outward by margin; the render area
// (bounds) shrinks inward. measure/arrange reserve the layout box (content + margin);
// child bounds are the box minus margin.
void test_invariant_margin() {
    // StackPanel: child margin 10 → layout box 60×50
    StackPanel sp(Orientation::Vertical);
    sp.set_padding(0.0f);
    sp.set_spacing(0.0f);
    FixedWidget s1(Size{40.0f, 30.0f});
    s1.set_margin(10.0f);
    FixedWidget s2(Size{20.0f, 20.0f});
    sp.append_child(&s1);
    sp.append_child(&s2);
    CHECK(sp.measure(Size{200.0f, 200.0f}).width == 60.0f);
    CHECK(sp.measure(Size{200.0f, 200.0f}).height == 70.0f);
    sp.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    sp.perform_layout();
    // Render area shrinks inward by margin
    CHECK(s1.bounds().left == 10.0f && s1.bounds().top == 10.0f);
    // StackPanel stretches child width to the content area (minus margin)
    CHECK(s1.bounds().width() == 180.0f && s1.bounds().height() == 30.0f);
    // The next child starts after the layout box
    CHECK(s2.bounds().top == 50.0f);
    // Margin area doesn't hit; content area does
    CHECK(sp.hit_test(5.0f, 5.0f) == &sp);
    CHECK(sp.hit_test(15.0f, 15.0f) == &s1);

    // WrapPanel: layout box 80×60; width 170 fits 2 per row
    WrapPanel wp;
    wp.set_spacing(0.0f);
    wp.set_line_spacing(0.0f);
    FixedWidget w1(Size{60.0f, 40.0f});
    w1.set_margin(10.0f);
    FixedWidget w2(Size{60.0f, 40.0f});
    w2.set_margin(10.0f);
    FixedWidget w3(Size{60.0f, 40.0f});
    w3.set_margin(10.0f);
    wp.append_child(&w1);
    wp.append_child(&w2);
    wp.append_child(&w3);
    wp.set_bounds(RectF::make(0.0f, 0.0f, 170.0f, 200.0f));
    wp.perform_layout();
    CHECK(w1.bounds().left == 10.0f && w1.bounds().top == 10.0f);
    CHECK(w2.bounds().left == 90.0f && w2.bounds().top == 10.0f);
    CHECK(w3.bounds().left == 10.0f && w3.bounds().top == 70.0f);

    // GridPanel: column width = content + margin = 60; child bounds shrink inward
    GridPanel gp(1, 1);
    gp.set_gap(0.0f);
    FixedWidget g1(Size{40.0f, 30.0f});
    g1.set_margin(10.0f);
    gp.add(&g1, 0, 0);
    gp.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    gp.perform_layout();
    CHECK(g1.bounds().left == 10.0f && g1.bounds().top == 10.0f);
    CHECK(g1.bounds().width() == 40.0f && g1.bounds().height() == 30.0f);

    // DockPanel: Left layout box 80 wide; child bounds shrink inward
    DockPanel dp;
    FixedWidget d1(Size{60.0f, 40.0f});
    d1.set_margin(10.0f);
    FixedWidget d2(Size{40.0f, 40.0f});
    dp.dock(&d1, Dock::Left);
    dp.dock(&d2, Dock::Fill);
    dp.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    dp.perform_layout();
    CHECK(d1.bounds().left == 10.0f && d1.bounds().top == 10.0f);
    CHECK(d1.bounds().width() == 60.0f && d1.bounds().height() == 80.0f);
    CHECK(d2.bounds().left == 80.0f && d2.bounds().top == 0.0f);
    CHECK(d2.bounds().width() == 120.0f && d2.bounds().height() == 100.0f);
}

// General layout invariant: recursive nesting. Each level keeps correct local coords;
// global = sum along the parent chain, so hit_test cascades to the deepest child.
void test_invariant_recursive() {
    // 1. Wrap around Stack: Stack content 70×30; wrap 170 wide → 2 per row × 2 rows
    FixedWidget f1(Size{70.0f, 30.0f});
    FixedWidget f2(Size{70.0f, 30.0f});
    FixedWidget f3(Size{70.0f, 30.0f});
    FixedWidget f4(Size{70.0f, 30.0f});
    StackPanel s1(Orientation::Vertical), s2(Orientation::Vertical);
    StackPanel s3(Orientation::Vertical), s4(Orientation::Vertical);
    for (StackPanel* s : {&s1, &s2, &s3, &s4}) {
        s->set_padding(0.0f);
        s->set_spacing(0.0f);
    }
    s1.append_child(&f1);
    s2.append_child(&f2);
    s3.append_child(&f3);
    s4.append_child(&f4);

    WrapPanel wrap;
    wrap.set_spacing(10.0f);
    wrap.set_line_spacing(10.0f);
    wrap.append_child(&s1);
    wrap.append_child(&s2);
    wrap.append_child(&s3);
    wrap.append_child(&s4);
    wrap.set_bounds(RectF::make(0.0f, 0.0f, 170.0f, 200.0f));
    wrap.perform_layout();

    CHECK(s1.bounds().left == 0.0f && s1.bounds().top == 0.0f);
    CHECK(s1.bounds().width() == 70.0f && s1.bounds().height() == 30.0f);
    CHECK(s2.bounds().left == 80.0f && s2.bounds().top == 0.0f);
    CHECK(s3.bounds().left == 0.0f && s3.bounds().top == 40.0f);
    CHECK(s4.bounds().left == 80.0f && s4.bounds().top == 40.0f);
    CHECK(f1.bounds().left == 0.0f && f1.bounds().top == 0.0f);
    CHECK(f4.global_bounds().left == 80.0f && f4.global_bounds().top == 40.0f);
    CHECK(wrap.hit_test(81.0f, 41.0f) == &f4);
    CHECK(wrap.hit_test(5.0f, 5.0f) == &f1);

    // 2. Stack around Grid: 2×2, column widths 50/60, row heights 30/40; Stack stretches child width
    FixedWidget g1(Size{50.0f, 30.0f});
    FixedWidget g2(Size{60.0f, 30.0f});
    FixedWidget g3(Size{40.0f, 40.0f});
    FixedWidget g4(Size{20.0f, 20.0f});
    GridPanel grid(2, 2);
    grid.set_gap(0.0f);
    grid.add(&g1, 0, 0);
    grid.add(&g2, 1, 0);
    grid.add(&g3, 0, 1);
    grid.add(&g4, 1, 1);
    StackPanel stack(Orientation::Vertical);
    stack.set_padding(0.0f);
    stack.set_spacing(0.0f);
    stack.append_child(&grid);
    stack.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 200.0f));
    stack.perform_layout();

    CHECK(grid.bounds().left == 0.0f && grid.bounds().top == 0.0f);
    CHECK(grid.bounds().width() == 300.0f && grid.bounds().height() == 70.0f);
    CHECK(g1.bounds().left == 0.0f && g1.bounds().top == 0.0f);
    CHECK(g2.bounds().left == 50.0f && g2.bounds().top == 0.0f);
    CHECK(g3.bounds().left == 0.0f && g3.bounds().top == 30.0f);
    CHECK(g4.bounds().left == 50.0f && g4.bounds().top == 30.0f);
    CHECK(g4.global_bounds().left == 50.0f && g4.global_bounds().top == 30.0f);
    CHECK(stack.hit_test(51.0f, 31.0f) == &g4);

    // 3. Three levels: Wrap around Stack around Grid
    FixedWidget t1(Size{50.0f, 30.0f});
    FixedWidget t2(Size{60.0f, 30.0f});
    GridPanel tgrid(2, 1);
    tgrid.set_gap(0.0f);
    tgrid.add(&t1, 0, 0);
    tgrid.add(&t2, 1, 0);
    StackPanel tstack(Orientation::Vertical);
    tstack.set_padding(0.0f);
    tstack.set_spacing(0.0f);
    tstack.append_child(&tgrid);
    WrapPanel twrap;
    twrap.set_spacing(0.0f);
    twrap.set_line_spacing(0.0f);
    twrap.append_child(&tstack);
    twrap.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    twrap.perform_layout();

    CHECK(tgrid.bounds().left == 0.0f && tgrid.bounds().top == 0.0f);
    CHECK(tgrid.bounds().width() == 110.0f && tgrid.bounds().height() == 30.0f);
    CHECK(t2.bounds().left == 50.0f && t2.bounds().top == 0.0f);
    CHECK(t2.global_bounds().left == 50.0f && t2.global_bounds().top == 0.0f);
    CHECK(twrap.hit_test(51.0f, 1.0f) == &t2);
}

// General layout invariant: window resize. After a size change, re-Measure+Arrange
// must update every child to the new size.
void test_invariant_resize() {
    // 1. WrapPanel: width 170 (3/row) → 350 (6/row) → back to 170
    WrapPanel wrap;
    wrap.set_spacing(10.0f);
    wrap.set_line_spacing(10.0f);
    FixedWidget a(Size{50.0f, 30.0f});
    FixedWidget b(Size{50.0f, 30.0f});
    FixedWidget c(Size{50.0f, 30.0f});
    FixedWidget d(Size{50.0f, 30.0f});
    FixedWidget e(Size{50.0f, 30.0f});
    FixedWidget f(Size{50.0f, 30.0f});
    wrap.append_child(&a);
    wrap.append_child(&b);
    wrap.append_child(&c);
    wrap.append_child(&d);
    wrap.append_child(&e);
    wrap.append_child(&f);

    wrap.set_bounds(RectF::make(0.0f, 0.0f, 170.0f, 200.0f));
    wrap.perform_layout();
    CHECK(a.bounds().left == 0.0f && a.bounds().top == 0.0f);
    CHECK(c.bounds().left == 120.0f && c.bounds().top == 0.0f);
    CHECK(d.bounds().left == 0.0f && d.bounds().top == 40.0f);

    wrap.set_bounds(RectF::make(0.0f, 0.0f, 350.0f, 200.0f));
    wrap.perform_layout();
    CHECK(d.bounds().top == 0.0f);
    CHECK(f.bounds().left == 300.0f && f.bounds().top == 0.0f);

    wrap.set_bounds(RectF::make(0.0f, 0.0f, 170.0f, 200.0f));
    wrap.perform_layout();
    CHECK(d.bounds().left == 0.0f && d.bounds().top == 40.0f);
    CHECK(f.bounds().left == 120.0f && f.bounds().top == 40.0f);

    // 2. GridPanel: star 1:1 columns re-split with container width
    GridPanel gp(2, 1);
    gp.set_gap(0.0f);
    FixedWidget g1(Size{50.0f, 30.0f});
    FixedWidget g2(Size{60.0f, 30.0f});
    gp.add(&g1, 0, 0);
    gp.add(&g2, 1, 0);
    gp.set_column_star(0, 1.0f);
    gp.set_column_star(1, 1.0f);
    gp.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 100.0f));
    gp.perform_layout();
    CHECK(g1.bounds().width() == 145.0f);
    CHECK(g2.bounds().left == 145.0f);
    gp.set_bounds(RectF::make(0.0f, 0.0f, 400.0f, 100.0f));
    gp.perform_layout();
    CHECK(g1.bounds().width() == 195.0f);
    CHECK(g2.bounds().left == 195.0f);

    // 3. DockPanel: size change → Left height / Fill area update
    DockPanel dp;
    FixedWidget l(Size{60.0f, 40.0f});
    FixedWidget fl(Size{40.0f, 40.0f});
    dp.dock(&l, Dock::Left);
    dp.dock(&fl, Dock::Fill);
    dp.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    dp.perform_layout();
    CHECK(fl.bounds().width() == 140.0f);
    dp.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 150.0f));
    dp.perform_layout();
    CHECK(l.bounds().height() == 150.0f);
    CHECK(fl.bounds().left == 60.0f && fl.bounds().width() == 240.0f);
    CHECK(fl.bounds().height() == 150.0f);

    // 4. StackPanel: width change → stretched child widths update
    StackPanel sp(Orientation::Vertical);
    sp.set_padding(0.0f);
    sp.set_spacing(0.0f);
    FixedWidget s1(Size{40.0f, 30.0f});
    sp.append_child(&s1);
    sp.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    sp.perform_layout();
    CHECK(s1.bounds().width() == 200.0f);
    sp.set_bounds(RectF::make(0.0f, 0.0f, 350.0f, 100.0f));
    sp.perform_layout();
    CHECK(s1.bounds().width() == 350.0f);
}

// General layout invariant: empty panel. 0 children → DesiredSize (0,0), perform_layout must not crash.
void test_invariant_empty() {
    // StackPanel
    StackPanel sp(Orientation::Vertical);
    CHECK(sp.measure(Size{300.0f, 200.0f}).width == 0.0f);
    CHECK(sp.measure(Size{300.0f, 200.0f}).height == 0.0f);
    sp.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 200.0f));
    sp.perform_layout();
    CHECK(sp.measure(Size{0.0f, 0.0f}).width == 0.0f);

    // WrapPanel
    WrapPanel wp;
    CHECK(wp.measure(Size{300.0f, 200.0f}).width == 0.0f);
    CHECK(wp.measure(Size{300.0f, 200.0f}).height == 0.0f);
    wp.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 200.0f));
    wp.perform_layout();

    // GridPanel (must be (0,0) even with a gap)
    GridPanel gp(3, 2);
    gp.set_gap(10.0f);
    CHECK(gp.measure(Size{300.0f, 200.0f}).width == 0.0f);
    CHECK(gp.measure(Size{300.0f, 200.0f}).height == 0.0f);
    gp.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 200.0f));
    gp.perform_layout();

    // DockPanel
    DockPanel dp;
    CHECK(dp.measure(Size{300.0f, 200.0f}).width == 0.0f);
    CHECK(dp.measure(Size{300.0f, 200.0f}).height == 0.0f);
    dp.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 200.0f));
    dp.perform_layout();

    // Empty panels nested in a Stack don't crash either
    StackPanel outer(Orientation::Vertical);
    outer.set_padding(0.0f);
    outer.set_spacing(0.0f);
    outer.append_child(&sp);
    outer.append_child(&wp);
    outer.set_bounds(RectF::make(0.0f, 0.0f, 300.0f, 200.0f));
    outer.perform_layout();
    CHECK(outer.measure(Size{300.0f, 200.0f}).width == 0.0f);
    CHECK(outer.measure(Size{300.0f, 200.0f}).height == 0.0f);
}

// Visibility: hidden children take no space (skipped by measure/arrange), don't hit-test, and don't affect layout
void test_invariant_visibility() {
    // StackPanel: hidden child takes no space → later children unaffected
    StackPanel sp(Orientation::Vertical);
    sp.set_padding(0.0f);
    sp.set_spacing(0.0f);
    FixedWidget a(Size{40.0f, 30.0f});
    FixedWidget b(Size{40.0f, 30.0f});
    FixedWidget c(Size{40.0f, 30.0f});
    sp.append_child(&a);
    sp.append_child(&b);
    sp.append_child(&c);
    b.set_visible(false);
    sp.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 200.0f));
    sp.perform_layout();
    CHECK(a.bounds().top == 0.0f);
    CHECK(c.bounds().top == 30.0f);
    CHECK(sp.measure(Size{200.0f, 200.0f}).height == 60.0f);
    CHECK(sp.hit_test(5.0f, 35.0f) == &c);

    // WrapPanel: hidden child takes no space and doesn't wrap
    WrapPanel wp;
    wp.set_spacing(0.0f);
    wp.set_line_spacing(0.0f);
    FixedWidget w1(Size{50.0f, 30.0f});
    FixedWidget w2(Size{50.0f, 30.0f});
    FixedWidget w3(Size{50.0f, 30.0f});
    wp.append_child(&w1);
    wp.append_child(&w2);
    wp.append_child(&w3);
    w2.set_visible(false);
    wp.set_bounds(RectF::make(0.0f, 0.0f, 110.0f, 200.0f));
    wp.perform_layout();
    CHECK(w3.bounds().left == 50.0f && w3.bounds().top == 0.0f);

    // GridPanel: hidden child occupies no cell
    GridPanel gp(2, 1);
    gp.set_gap(0.0f);
    FixedWidget g1(Size{50.0f, 30.0f});
    FixedWidget g2(Size{60.0f, 30.0f});
    gp.add(&g1, 0, 0);
    gp.add(&g2, 1, 0);
    g2.set_visible(false);
    gp.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    gp.perform_layout();
    CHECK(g1.bounds().width() == 50.0f);
    CHECK(gp.measure(Size{200.0f, 200.0f}).width == 50.0f);

    // DockPanel: hidden child takes no space → Fill starts after visible children
    DockPanel dp;
    FixedWidget d1(Size{60.0f, 40.0f});
    FixedWidget d2(Size{40.0f, 40.0f});
    FixedWidget d3(Size{40.0f, 40.0f});
    dp.dock(&d1, Dock::Left);
    dp.dock(&d2, Dock::Left);
    dp.dock(&d3, Dock::Fill);
    d2.set_visible(false);
    dp.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    dp.perform_layout();
    CHECK(d3.bounds().left == 60.0f);
}

// Overflow: children exceed the container without crashing; placed positions stay correct
void test_invariant_overflow() {
    // StackPanel height overflow: 150 > 100
    StackPanel sp(Orientation::Vertical);
    sp.set_padding(0.0f);
    sp.set_spacing(0.0f);
    FixedWidget a(Size{40.0f, 50.0f});
    FixedWidget b(Size{40.0f, 50.0f});
    FixedWidget c(Size{40.0f, 50.0f});
    sp.append_child(&a);
    sp.append_child(&b);
    sp.append_child(&c);
    sp.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    sp.perform_layout();
    CHECK(a.bounds().top == 0.0f && b.bounds().top == 50.0f);
    CHECK(c.bounds().top == 100.0f && c.bounds().height() == 50.0f);

    // WrapPanel width overflow: 80 > 60 → children clamped to available
    WrapPanel wp;
    wp.set_spacing(0.0f);
    wp.set_line_spacing(0.0f);
    FixedWidget w1(Size{80.0f, 30.0f});
    FixedWidget w2(Size{80.0f, 30.0f});
    wp.append_child(&w1);
    wp.append_child(&w2);
    wp.set_bounds(RectF::make(0.0f, 0.0f, 60.0f, 200.0f));
    wp.perform_layout();
    CHECK(w1.bounds().width() == 60.0f);
    CHECK(w2.bounds().width() == 60.0f);
    CHECK(w2.bounds().top == 30.0f);

    // GridPanel content exceeds container: no crash, cells placed regardless
    GridPanel gp(2, 1);
    gp.set_gap(0.0f);
    FixedWidget g1(Size{50.0f, 30.0f});
    FixedWidget g2(Size{60.0f, 30.0f});
    gp.add(&g1, 0, 0);
    gp.add(&g2, 1, 0);
    gp.set_bounds(RectF::make(0.0f, 0.0f, 60.0f, 100.0f));
    gp.perform_layout();
    CHECK(g1.bounds().width() == 50.0f);
    CHECK(g2.bounds().left == 50.0f);

    // DockPanel horizontal overflow: 160 > 100; Fill is squeezed to 0
    DockPanel dp;
    FixedWidget d1(Size{80.0f, 40.0f});
    FixedWidget d2(Size{80.0f, 40.0f});
    FixedWidget d3(Size{40.0f, 40.0f});
    dp.dock(&d1, Dock::Left);
    dp.dock(&d2, Dock::Left);
    dp.dock(&d3, Dock::Fill);
    dp.set_bounds(RectF::make(0.0f, 0.0f, 100.0f, 100.0f));
    dp.perform_layout();
    CHECK(d1.bounds().width() == 80.0f);
    CHECK(d3.bounds().width() == 0.0f);
}

// stretch_children: when false, children keep their content size (no stretching)
void test_stretch_children() {
    StackPanel sp(Orientation::Vertical);
    sp.set_padding(0.0f);
    sp.set_spacing(0.0f);
    sp.set_stretch_children(false);
    FixedWidget a(Size{40.0f, 30.0f});
    sp.append_child(&a);
    sp.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    sp.perform_layout();
    CHECK(a.bounds().width() == 40.0f && a.bounds().left == 0.0f);

    StackPanel sp2(Orientation::Horizontal);
    sp2.set_padding(0.0f);
    sp2.set_spacing(0.0f);
    sp2.set_stretch_children(false);
    FixedWidget b(Size{40.0f, 30.0f});
    sp2.append_child(&b);
    sp2.set_bounds(RectF::make(0.0f, 0.0f, 200.0f, 100.0f));
    sp2.perform_layout();
    CHECK(b.bounds().height() == 30.0f && b.bounds().top == 0.0f);
}

// Demo simulation: ScrollView → StackPanel content → star-row grid (no Label, no ctx)
// void test_scroll_star_content() {
//     ScrollView scroll;
//     scroll.set_bounds(RectF::make(0.0f, 0.0f, 400.0f, 300.0f));
//     StackPanel content;
//     content.set_spacing(8.0f);
//     scroll.set_content(&content);
//
//     GridPanel grid2(1, 3);
//     grid2.set_gap(8.0f);
//     grid2.set_row_auto(0);
//     grid2.set_row_star(1, 1.0f);
//     grid2.set_row_star(2, 2.0f);
//     FixedWidget a(Size{60.0f, 32.0f});
//     FixedWidget b(Size{40.0f, 32.0f});
//     FixedWidget c(Size{40.0f, 32.0f});
//     grid2.add(&a, 0, 0);
//     grid2.add(&b, 0, 1);
//     grid2.add(&c, 0, 2);
//     content.append_child(&grid2);
//
//     scroll.perform_layout();
//
//     if (FILE* f = std::fopen("C:/Users/p1590/AppData/Local/Temp/opencode/layout_test_result.txt", "a")) {
//         std::fprintf(f,
//                      "DEBUG scroll: content_h=%f grid2=%f,%f,%f,%f auto=%f,%f,%f,%f star1=%f,%f,%f,%f star2=%f,%f,%f,%f\n",
//                      scroll.content()->height(), grid2.x(), grid2.y(), grid2.width(), grid2.height(),
//                      a.x(), a.y(), a.width(), a.height(),
//                      b.x(), b.y(), b.width(), b.height(),
//                      c.x(), c.y(), c.width(), c.height());
//         std::fclose(f);
//     }
//     CHECK(grid2.height() == 112.0f);
//     CHECK(c.height() == 32.0f);
// }

}  // namespace

int main() {
    test_stack_vertical();
    test_stack_horizontal();
    test_margin();
    test_zero_size();
    test_available_size();
    test_wrap_panel_wrap();
    test_wrap_panel_gap();
    test_wrap_panel_demo();
    test_wrap_boundary();
    test_wrap_single();
    test_wrap_measure_arrange_consistency();
    test_grid_fixed();
    test_grid_star_columns();
    test_grid_span();
    test_grid_star_rows();
    test_grid_star_rows_ratio();
    test_grid_star_rows_gap();
    test_grid_star_rows_span();
    test_grid_star_fill();
    test_grid_star_fill_stack();
    test_measure_min_max();
    test_dock_basic();
    test_dock_stacking();
    test_dock_gap();
    test_invariant_local_coords();
    test_invariant_desired_size();
    test_invariant_margin();
    test_invariant_recursive();
    test_invariant_resize();
    test_invariant_empty();
    test_invariant_visibility();
    test_invariant_overflow();
    test_stretch_children();

    if (FILE* f = std::fopen("C:/Users/p1590/AppData/Local/Temp/opencode/layout_test_result.txt", "a")) {
        std::fprintf(f, "%d checks, %d failures\n", g_checks, g_failures);
        std::fclose(f);
    }

    Application& app = Application::instance();

    Window window("YuzukiUI Layout Test", 1024, 720);
    if (!window.create()) return 1;

    {
        wchar_t exe_path[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        const std::wstring dir(exe_path, wcsrchr(exe_path, L'\\') + 1);
        window.backend().add_font_file(utf::to_utf8(dir + L"Satoshi-Regular.otf"));
    }

    auto content = std::make_unique<StackPanel>(Orientation::Vertical);
    content->set_padding(0.0f);
    content->set_spacing(12.0f);

    auto result = new Label(g_failures == 0
                                ? "PASS: " + std::to_string(g_checks) + " checks, 0 failures"
                                : "FAIL: " + std::to_string(g_failures) + " failures");
    result->set_align(TextAlignH::Left, TextAlignV::Center);
    content->append_child(result);

    auto vnote = new Label("vertical StackPanel: spacing 8, padding 0");
    vnote->set_small(true);
    vnote->set_align(TextAlignH::Left, TextAlignV::Center);
    content->append_child(vnote);

    content->append_child(new FixedWidget(Size{80.0f, 32.0f}, Color{0x5A, 0x8F, 0xFF}, "cell A"));
    content->append_child(new FixedWidget(Size{80.0f, 32.0f}, Color{0x8A, 0x7C, 0xCC}, "cell B"));
    content->append_child(new FixedWidget(Size{80.0f, 32.0f}, Color{0x4C, 0xAF, 0x8A}, "cell C"));

    auto hnote = new Label("horizontal StackPanel: spacing 10");
    hnote->set_small(true);
    hnote->set_align(TextAlignH::Left, TextAlignV::Center);
    content->append_child(hnote);

    auto hstack = new StackPanel(Orientation::Horizontal);
    hstack->set_padding(0.0f);
    hstack->set_spacing(10.0f);
    hstack->append_child(new FixedWidget(Size{80.0f, 32.0f}, Color{0x5A, 0x8F, 0xFF}, "stack A"));
    hstack->append_child(new FixedWidget(Size{50.0f, 32.0f}, Color{0x8A, 0x7C, 0xCC}, "stack B"));
    hstack->append_child(new FixedWidget(Size{80.0f, 32.0f}, Color{0x4C, 0xAF, 0x8A}, "stack C"));
    content->append_child(hstack);

    auto mnote = new Label("child with margin 40");
    mnote->set_small(true);
    mnote->set_align(TextAlignH::Left, TextAlignV::Center);
    content->append_child(mnote);

    auto mhost = new StackPanel(Orientation::Vertical);
    mhost->set_padding(0.0f);
    mhost->set_spacing(0.0f);
    mhost->set_margin(Margins{40.0f, 40.0f, 40.0f, 40.0f});
    mhost->append_child(new FixedWidget(Size{80.0f, 32.0f}, Color{0xE0, 0x8A, 0x4C}, "margined"));
    content->append_child(mhost);

    auto wnote = new Label("WrapPanel: spacing 8, line_spacing 8");
    wnote->set_small(true);
    wnote->set_align(TextAlignH::Left, TextAlignV::Center);
    content->append_child(wnote);

    auto wrap = new WrapPanel;
    wrap->set_spacing(8.0f);
    wrap->set_line_spacing(8.0f);
    static const Color colors[] = {Color{0x5A, 0x8F, 0xFF}, Color{0x8A, 0x7C, 0xCC},
                                   Color{0x4C, 0xAF, 0x8A}, Color{0xE0, 0x8A, 0x4C}};
    for (int i = 0; i < 14; ++i) {
        wrap->append_child(new FixedWidget(
            Size{90.0f, 28.0f}, colors[i % 4],
            "item " + std::to_string(i + 1)));
    }
    content->append_child(wrap);

    auto gnote = new Label("GridPanel: col0 auto | col1 star(1) | col2 star(2), gap 8");
    gnote->set_small(true);
    gnote->set_align(TextAlignH::Left, TextAlignV::Center);
    content->append_child(gnote);

    auto grid = new GridPanel(3, 2);
    grid->set_gap(8.0f);
    grid->set_column_auto(0);
    grid->set_column_star(1, 1.0f);
    grid->set_column_star(2, 2.0f);
    grid->add(new FixedWidget(Size{60.0f, 32.0f}, Color{0x5A, 0x8F, 0xFF}, "auto"), 0, 0);
    grid->add(new FixedWidget(Size{40.0f, 32.0f}, Color{0x8A, 0x7C, 0xCC}, "star1"), 1, 0);
    grid->add(new FixedWidget(Size{40.0f, 32.0f}, Color{0x4C, 0xAF, 0x8A}, "star2"), 2, 0);
    grid->add(new FixedWidget(Size{60.0f, 40.0f}, Color{0xE0, 0x8A, 0x4C}, "span 2x1"), 0, 1, 2, 1);
    grid->add(new FixedWidget(Size{30.0f, 40.0f}, Color{0xD0, 0x5A, 0x8F}, "star"), 2, 1);
    content->append_child(grid);

    auto dnote = new Label("DockPanel: left | top | right | bottom | fill (chat layout)");
    dnote->set_small(true);
    dnote->set_align(TextAlignH::Left, TextAlignV::Center);
    content->append_child(dnote);

    auto dock = new DockPanel;
    dock->dock(new FixedWidget(Size{120.0f, 40.0f}, Color{0x5A, 0x8F, 0xFF}, "left"), Dock::Left);
    dock->dock(new FixedWidget(Size{40.0f, 44.0f}, Color{0x8A, 0x7C, 0xCC}, "top"), Dock::Top);
    dock->dock(new FixedWidget(Size{90.0f, 40.0f}, Color{0x4C, 0xAF, 0x8A}, "right"), Dock::Right);
    dock->dock(new FixedWidget(Size{40.0f, 30.0f}, Color{0xE0, 0x8A, 0x4C}, "bottom"), Dock::Bottom);
    dock->dock(new FixedWidget(Size{40.0f, 300.0f}, Color{0xD0, 0x5A, 0x8F}, "fill"), Dock::Fill);
    content->append_child(dock);

    auto dnote2 = new Label("DockPanel with gap 12");
    dnote2->set_small(true);
    dnote2->set_align(TextAlignH::Left, TextAlignV::Center);
    content->append_child(dnote2);

    auto dock2 = new DockPanel;
    dock2->set_gap(12.0f);
    dock2->dock(new FixedWidget(Size{100.0f, 40.0f}, Color{0x5A, 0x8F, 0xFF}, "left"), Dock::Left);
    dock2->dock(new FixedWidget(Size{40.0f, 44.0f}, Color{0x8A, 0x7C, 0xCC}, "top"), Dock::Top);
    dock2->dock(new FixedWidget(Size{80.0f, 40.0f}, Color{0x4C, 0xAF, 0x8A}, "right"), Dock::Right);
    dock2->dock(new FixedWidget(Size{40.0f, 28.0f}, Color{0xE0, 0x8A, 0x4C}, "bottom"), Dock::Bottom);
    dock2->dock(new FixedWidget(Size{40.0f, 240.0f}, Color{0xD0, 0x5A, 0x8F}, "fill"), Dock::Fill);
    content->append_child(dock2);

    auto gnote2 = new Label("GridPanel rows: row0 auto | row1 star(1) | row2 star(2), gap 8");
    gnote2->set_small(true);
    gnote2->set_align(TextAlignH::Left, TextAlignV::Center);
    content->append_child(gnote2);

    auto grid2 = new GridPanel(1, 3);
    grid2->set_gap(8.0f);
    grid2->set_row_auto(0);
    grid2->set_row_star(1, 1.0f);
    grid2->set_row_star(2, 2.0f);
    grid2->add(new FixedWidget(Size{60.0f, 32.0f}, Color{0x5A, 0x8F, 0xFF}, "auto"), 0, 0);
    grid2->add(new FixedWidget(Size{40.0f, 32.0f}, Color{0x8A, 0x7C, 0xCC}, "star1"), 0, 1);
    grid2->add(new FixedWidget(Size{40.0f, 32.0f}, Color{0x4C, 0xAF, 0x8A}, "star2"), 0, 2);
    content->append_child(grid2);

    auto scroll = std::make_unique<ScrollView>();
    scroll->set_content(content.get());

    window.set_root(scroll.get());
    window.show();

    return app.run();
}