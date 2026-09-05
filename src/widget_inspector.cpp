#include <yuzuki/ui/widget_inspector.hpp>
#include <yuzuki/ui/window.hpp>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <typeinfo>

namespace yzk {

namespace {

constexpr u32 kPollIntervalMs = 500;
constexpr f32 kPadX = 10.0f;
constexpr f32 kPadY = 8.0f;
constexpr f32 kRowH = 16.0f;
constexpr f32 kIndent = 12.0f;
constexpr f32 kHeaderH = 26.0f;
constexpr f32 kPropH = 17.0f;

// "class yzk::Button" -> "Button"; "struct yzk::Row" -> "Row".
String short_type_name(const Widget* w) {
    const char* raw = typeid(*w).name();
    const char* name = raw;
    const char* sp = std::strchr(raw, ' ');
    if (sp) name = sp + 1;
    if (const char* scope = std::strchr(name, ':'); scope) name = scope + 1;
    return String(name);
}

String format_rect(const RectF& r) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "(%.0f, %.0f) %.0f x %.0f", r.left, r.top, r.width(), r.height());
    return String(buf);
}

}  // namespace

WidgetInspector::WidgetInspector() = default;
WidgetInspector::~WidgetInspector() = default;

void WidgetInspector::show(Window& win) {
    if (is_open_) return;
    Widget* root = win.root();
    if (!root) return;

    win_ = &win;
    is_open_ = true;
    rows_.clear();
    rows_.push_back(Row{&win, 0});
    collect(root, 1);
    root->append_child(this);
    win_->start_timer(this, kPollIntervalMs);
    win.invalidate_all();
}

void WidgetInspector::close() {
    if (!is_open_) return;
    is_open_ = false;
    if (win_) {
        win_->stop_timer(this);
        win_->invalidate_all();
    }
    remove_from_parent();
    rows_.clear();
    selected_ = SIZE_MAX;
    win_ = nullptr;
}

void WidgetInspector::collect(Widget* w, u32 depth) {
    if (!w) return;
    rows_.push_back(Row{w, depth});
    for (Widget* c = w->first_child(); c; c = c->next_sibling()) collect(c, depth + 1);
}

const WidgetInspector::Row* WidgetInspector::hit_row(f32 local_x, f32 local_y) const {
    if (!panel_.contains(local_x, local_y)) return nullptr;
    const f32 tree_top = panel_.top + kHeaderH;
    const f32 ty = local_y - tree_top;
    const size_t idx = static_cast<size_t>(ty / kRowH);
    if (ty < 0.0f || idx >= rows_.size()) return nullptr;
    if (local_x >= panel_.left + kPadX && local_x <= panel_.left + panel_.width() - kPadX)
        return &rows_[idx];
    return nullptr;
}

void WidgetInspector::perform_layout(const PaintContext* ctx) {
    if (win_) set_bounds(win_->bounds());
    Widget::perform_layout(ctx);
}

void WidgetInspector::paint_impl(PaintContext& ctx) {
    const f32 ox = ctx.offset_x(), oy = ctx.offset_y();
    ctx.set_offset(ox + bounds_.left, oy + bounds_.top);

    // Build every text line up front so the panel can size itself to the widest
    // row (labels, tree indent, and property rows) instead of clipping.
    std::vector<String> lines;
    lines.push_back("Widget Tree  (F2)");
    for (const Row& r : rows_) {
        const String label = r.widget && r.widget->is_window()
                                 ? String("root")
                                 : r.widget ? short_type_name(r.widget) : String("(null)");
        lines.push_back(label);
    }

    Widget* sel = nullptr;
    if (selected_ < rows_.size()) sel = rows_[selected_].widget;

    std::vector<std::pair<String, f32>> props;  // text, x-offset from panel left
    if (sel) {
        const RectF gb = sel->global_bounds();
        const Margins& m = sel->margin();
        char buf[192];

        props.push_back({"-- " + short_type_name(sel) + " --", 0.0f});
        std::snprintf(buf, sizeof(buf), "Bounds  local %s  global %s", format_rect(sel->bounds()).c_str(),
                      format_rect(gb).c_str());
        props.push_back({buf, 0.0f});
        std::snprintf(buf, sizeof(buf), "Margin  L%.0f T%.0f R%.0f B%.0f", m.left, m.top, m.right, m.bottom);
        props.push_back({buf, 0.0f});

        const Size& mn = sel->min_size(), mx = sel->max_size();
        std::snprintf(buf, sizeof(buf), "Size  min %.0fx%.0f  max %.0fx%.0f  grow %.1f shrink %.1f", mn.width,
                      mn.height, mx.width, mx.height, sel->flex_grow(), sel->flex_shrink());
        props.push_back({buf, 0.0f});

        std::snprintf(buf, sizeof(buf),
                      "State  vis%d en%d focusable%d hover%d pressed%d opacity %.2f", sel->visible() ? 1 : 0,
                      sel->enabled() ? 1 : 0, sel->focusable() ? 1 : 0, sel->hovered() ? 1 : 0,
                      sel->pressed() ? 1 : 0, sel->opacity());
        props.push_back({buf, 0.0f});

        std::snprintf(buf, sizeof(buf), "Desired  %.0fx%.0f  measure_ms %.3f", sel->desired_size().width,
                      sel->desired_size().height,
#ifdef _DEBUG
                      sel->measure_ms()
#else
                      0.0f
#endif
        );
        props.push_back({buf, 0.0f});

        const Widget* p = sel->parent();
        const String parent = p ? short_type_name(p) : String("(none)");
        std::snprintf(buf, sizeof(buf), "Parents  %s", parent.c_str());
        props.push_back({buf, 0.0f});

        u32 child_count = 0;
        for (Widget* c = sel->first_child(); c; c = c->next_sibling()) ++child_count;
        std::snprintf(buf, sizeof(buf), "Children  %u", child_count);
        props.push_back({buf, 0.0f});
    }

    // Panel width: widest of (indented tree label, header, property row) + padding.
    f32 need_w = ctx.measure_text(lines[0], true).width;
    for (const auto& pr : props) {
        const f32 tw = ctx.measure_text(pr.first, true).width;
        if (tw > need_w) need_w = tw;
    }
    for (size_t i = 1; i < lines.size(); ++i) {
        const f32 indent = kPadX + rows_[i - 1].depth * kIndent;
        const f32 tw = indent + ctx.measure_text(lines[i], true).width;
        if (tw > need_w) need_w = tw;
    }
    const f32 w = need_w + kPadX * 2.0f;
    panel_w_ = w;
    const f32 panel_w = w;

    // Panel height: tree rows + header + property section.
    const f32 tree_h = static_cast<f32>(rows_.size()) * kRowH;
    f32 h = kPadY + kHeaderH + tree_h + kPropH;
    if (sel) h += static_cast<f32>(props.size()) * kPropH;
    panel_ = RectF::make(8.0f, 8.0f, panel_w, h);

    ctx.fill_rounded(panel_, backdrop_, 6.0f);
    ctx.draw_border(panel_, Color{0xFF, 0xFF, 0xFF, 40}, 1.0f, 6.0f);

    // Header
    ctx.draw_text_small(lines[0], RectF::make(panel_.left + kPadX, panel_.top + 4.0f,
                                              panel_w - kPadX * 2.0f, kHeaderH),
                        accent_color_);

    // Selected widget highlight in the real window
    if (sel) {
        const RectF gb = sel->global_bounds();
        ctx.push_clip(bounds_);
        ctx.draw_border(gb, accent_color_, 1.5f, 2.0f);
        ctx.pop_clip();
    }

    // Tree rows
    f32 y = panel_.top + kHeaderH;
    for (size_t i = 1; i < lines.size(); ++i, y += kRowH) {
        const size_t idx = i - 1;
        const Row& r = rows_[idx];
        const f32 indent = kPadX + static_cast<f32>(r.depth) * kIndent;
        const bool sel_row = (idx == selected_);
        const bool is_window = r.widget && r.widget->is_window();
        const Color color = sel_row ? accent_color_
                                    : (r.depth == 0 ? Color{0xFF, 0xFF, 0xFF, 255}
                                                    : (is_window ? Color{0xDD, 0xDD, 0xDD, 255}
                                                                 : dim_color_));
        ctx.draw_text_small(lines[i], RectF::make(indent, y, panel_w - indent - kPadX, kRowH), color,
                            TextAlignH::Left, TextAlignV::Center);
        if (sel_row) {
            ctx.draw_border(RectF::make(panel_.left + 2.0f, y, panel_w - 4.0f, kRowH), accent_color_, 1.0f,
                            3.0f);
        }
    }

    // Property rows
    if (sel) {
        for (const auto& pr : props) {
            const bool header = pr.first.rfind("-- ", 0) == 0;
            ctx.draw_text_small(pr.first, RectF::make(panel_.left + kPadX, y, panel_w - kPadX * 2.0f, kPropH),
                                header ? Color{0xFF, 0xFF, 0xFF, 255} : dim_color_, TextAlignH::Left,
                                TextAlignV::Center);
            y += kPropH;
        }
    }

    ctx.set_offset(ox, oy);
}

void WidgetInspector::on_event(Event& e) {
    if (e.type == EventType::Timer) {
        e.consumed = true;
        // Preserve the selection across refresh: keep the selected widget by
        // identity, re-locate it in the freshly collected tree.
        Widget* keep = (selected_ < rows_.size()) ? rows_[selected_].widget : nullptr;
        rows_.clear();
        selected_ = SIZE_MAX;
        if (win_) {
            rows_.push_back(Row{win_, 0});
            collect(win_->root(), 1);
            if (keep) {
                for (size_t i = 0; i < rows_.size(); ++i) {
                    if (rows_[i].widget == keep) {
                        selected_ = i;
                        break;
                    }
                }
            }
        }
        invalidate();
        return;
    }
    if (e.type == EventType::MouseDown && (e.data.mouse.buttons & MouseButton_Left)) {
        const RectF g = global_bounds();
        const f32 lx = e.data.mouse.x - g.left;
        const f32 ly = e.data.mouse.y - g.top;
        if (const Row* hit = hit_row(lx, ly)) {
            e.consumed = true;
            selected_ = static_cast<size_t>(hit - rows_.data());
            invalidate();
            return;
        }
    }
    Widget::on_event(e);
}

}  // namespace yzk