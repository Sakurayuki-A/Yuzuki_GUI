#include <yuzuki/controls/context_menu.hpp>
#include <yuzuki/ui/window.hpp>
#include <yuzuki/ui/animation.hpp>

namespace yzk {

namespace {

constexpr f32 kItemHeight = 30.0f;
constexpr f32 kHozPadding = 26.0f;
constexpr f32 kSepHeight = 7.0f;
constexpr f32 kRadius = 8.0f;
constexpr f32 kFadeMs = 150.0f;
constexpr f32 kGap = 4.0f;

}  // namespace

void ContextMenu::add_item(const String& text, std::function<void()> action) {
    items_.push_back(ContextMenuItem{text, std::move(action)});
    invalidate();
}

void ContextMenu::add_separator() {
    items_.push_back(ContextMenuItem{String{}, nullptr, true});
    invalidate();
}

void ContextMenu::clear_items() {
    items_.clear();
    hover_ = -1;
    invalidate();
}

Size ContextMenu::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    f32 max_w = 0.0f;
    for (const ContextMenuItem& item : items_) {
        if (item.separator) continue;
        max_w = std::max(max_w, ctx->measure_text(item.text, false).width);
    }
    f32 h = 0.0f;
    for (const ContextMenuItem& item : items_) {
        h += item.separator ? kSepHeight : kItemHeight;
    }
    return Size{max_w + kHozPadding * 2.0f, h + kGap * 2.0f};
}

void ContextMenu::paint_impl(PaintContext& ctx) {
    const f32 alpha = progress_;
    const Theme& theme = Theme::get();

    ctx.draw_shadow(bounds_, kRadius, 14.0f, Color{0x00, 0x00, 0x00, (u8)(70 * alpha)});
    ctx.fill_rounded(bounds_, theme.surface.with_alpha((u8)(255 * alpha)), kRadius);
    ctx.draw_border(bounds_, theme.border.with_alpha((u8)(200 * alpha)), 1.0f, kRadius);

    f32 y = bounds_.top + kGap;
    for (size_t i = 0; i < items_.size(); ++i) {
        const ContextMenuItem& item = items_[i];
        if (item.separator) {
            const f32 line_y = y + kSepHeight * 0.5f;
            ctx.draw_line(Point{bounds_.left + 12.0f, line_y},
                          Point{bounds_.right - 12.0f, line_y},
                          theme.border.with_alpha((u8)(140 * alpha)), 1.0f);
            y += kSepHeight;
            continue;
        }
        const RectF row = RectF::make(bounds_.left + 4.0f, y, bounds_.width() - 8.0f, kItemHeight);
        const bool hot = hover_ == static_cast<i32>(i);
        if (hot && item.enabled) {
            ctx.fill_rounded(row, theme.accent.with_alpha((u8)(84 * alpha)), 6.0f);
        }
        const Color fg = hot && item.enabled ? theme.accent
                                             : (item.enabled ? theme.text : theme.text_disabled);
        ctx.draw_text(item.text,
                      RectF::make(bounds_.left + kHozPadding, y, bounds_.width() - kHozPadding * 2.0f,
                                  kItemHeight),
                      fg.with_alpha((u8)(230 * alpha)), TextAlignH::Left, TextAlignV::Center);
        y += kItemHeight;
    }
}

void ContextMenu::on_event(Event& e) {
    if (e.type == EventType::MouseMove) {
        e.consumed = true;
        const f32 lx = e.data.mouse.x - bounds_.left;
        const f32 ly = e.data.mouse.y - bounds_.top;
        const i32 idx = index_at(lx, ly);
        if (idx != hover_) {
            hover_ = idx;
            invalidate();
        }
        return;
    }
    if (e.type == EventType::MouseDown && (e.data.mouse.buttons & MouseButton_Left) != 0) {
        e.consumed = true;
        const f32 lx = e.data.mouse.x - bounds_.left;
        const f32 ly = e.data.mouse.y - bounds_.top;
        const i32 idx = index_at(lx, ly);
        if (idx >= 0 && items_[static_cast<size_t>(idx)].enabled) {
            auto action = items_[static_cast<size_t>(idx)].action;
            close();
            if (action) action();
        }
        return;
    }
    Widget::on_event(e);
}

Widget* ContextMenu::hit_test(f32 x, f32 y) {
    return (x >= 0.0f && y >= 0.0f && x <= bounds_.width() && y <= bounds_.height()) ? this
                                                                                     : nullptr;
}

i32 ContextMenu::index_at(f32 x, f32 y) const {
    if (x < 4.0f || x > bounds_.width() - 4.0f) return -1;
    f32 yy = y - kGap;
    for (size_t i = 0; i < items_.size(); ++i) {
        const f32 h = items_[i].separator ? kSepHeight : kItemHeight;
        if (yy >= 0.0f && yy < h && !items_[i].separator) return static_cast<i32>(i);
        yy -= h;
    }
    return -1;
}

void ContextMenu::perform_layout(const PaintContext* ctx) {
    if (!open_ || !ctx) return;
    measure(Size{1e7f, 1e7f}, ctx);
    const Size s = desired_size();
    Window* win = window();
    if (!win) return;
    const f32 win_w = win->bounds().width();
    const f32 win_h = win->bounds().height();
    f32 px = open_x_ + 8.0f;
    f32 py = open_y_ + 8.0f;
    if (px + s.width > win_w - 4.0f) px = open_x_ - s.width - 8.0f;
    if (py + s.height > win_h - 4.0f) py = open_y_ - s.height - 8.0f;
    px = std::max(0.0f, px);
    py = std::max(0.0f, py);
    set_bounds(RectF::make(px, py, s.width, s.height));
}

void ContextMenu::open(Window& win, f32 x, f32 y) {
    if (open_) return;
    open_ = true;
    open_x_ = x;
    open_y_ = y;
    progress_ = 0.0f;
    hover_ = -1;
    set_visible(true);
    Widget* root = win.root();
    if (root) root->append_child(this);
    win.set_context_menu(this);
    set_bounds(RectF::make(x + 8.0f, y + 8.0f, 160.0f, 100.0f));

    AnimationSystem::instance().tween(
        progress_, 1.0f, kFadeMs, Easing::OutCubic, [this](f32 v) {
            progress_ = v;
            invalidate();
        });
    win.invalidate_all();
}

void ContextMenu::close() {
    if (!open_) return;
    open_ = false;
    Window* win = window();
    set_visible(false);
    remove_from_parent();
    if (win) {
        win->set_context_menu(nullptr);
        win->invalidate_all();
    }
}

}  // namespace yzk