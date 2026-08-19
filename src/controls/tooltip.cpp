#include <yuzuki/controls/tooltip.hpp>
#include <yuzuki/ui/window.hpp>

namespace yzk {

namespace {

constexpr f32 kDelayMs = 500.0f;
constexpr f32 kFadeMs = 180.0f;
constexpr f32 kOffsetX = 12.0f;
constexpr f32 kOffsetY = 20.0f;

}  // namespace

// ===== Tooltip =====

void Tooltip::set_text(const String& text) {
    text_ = text;
    invalidate();
}

void Tooltip::set_anchor(f32 x, f32 y) {
    anchor_x_ = x;
    anchor_y_ = y;
    invalidate();
}

Size Tooltip::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    const Size text = ctx->measure_text(text_, true);
    size_w_ = text.width + 18.0f;
    size_h_ = text.height + 10.0f;
    return Size{size_w_, size_h_};
}

void Tooltip::paint_impl(PaintContext& ctx) {
    const u8 alpha = static_cast<u8>(228.0f * progress_);
    const Theme& theme = Theme::get();
    ctx.fill_rounded(bounds_, theme.surface_container_high.with_alpha(alpha), 6.0f);
    ctx.draw_text_small(text_, bounds_, theme.text.with_alpha(alpha), TextAlignH::Center,
                        TextAlignV::Center);
}

void Tooltip::on_event(Event& e) {
    if (e.type == EventType::Timer) {
        e.consumed = true;
        TooltipManager::instance().on_timer(*this);
        return;
    }
    Widget::on_event(e);
}

void Tooltip::perform_layout(const PaintContext* ctx) {
    if (!ctx) return;
    measure(Size{1e7f, 1e7f}, ctx);
    Window* win = window();
    if (!win) return;
    const f32 w = win->bounds().width();
    const f32 h = win->bounds().height();
    f32 x = anchor_x_ + kOffsetX;
    f32 y = anchor_y_ + kOffsetY;
    if (x + size_w_ > w - 8.0f) x = anchor_x_ - size_w_ - kOffsetX;
    if (y + size_h_ > h - 8.0f) y = anchor_y_ - size_h_ - kOffsetY;
    set_bounds(RectF::make(x, y, size_w_, size_h_));
}

// ===== TooltipManager =====

TooltipManager& TooltipManager::instance() {
    static TooltipManager manager;
    return manager;
}

void TooltipManager::set_tooltip(Widget* owner, const String& text) {
    for (Entry& entry : entries_) {
        if (entry.owner == owner) {
            entry.text = text;
            return;
        }
    }
    entries_.push_back(Entry{owner, text});
}

void TooltipManager::remove_tooltip(Widget* owner) {
    for (size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].owner == owner) {
            entries_.erase(entries_.begin() + static_cast<ptrdiff_t>(i));
            return;
        }
    }
}

Tooltip* TooltipManager::get_tip(Window& win) {
    auto it = tips_.find(&win);
    if (it != tips_.end()) return it->second;
    auto* tip = new Tooltip;
    tips_[&win] = tip;
    return tip;
}

void TooltipManager::on_hover_changed(Window& win, Widget* hovered, f32 mx, f32 my) {
    String text;
    Widget* owner = nullptr;
    if (hovered) {
        for (const Entry& entry : entries_) {
            if (entry.owner == hovered) {
                text = entry.text;
                owner = hovered;
                break;
            }
        }
    }

    if (owner != owner_) {
        hide(win);
        owner_ = owner;
        mx_ = 0.0f;
        my_ = 0.0f;
    }

    if (owner_ && owner_ == hovered) {
        mx_ = mx;
        my_ = my;
        Tooltip* tip = get_tip(win);
        if (!tip->visible()) {
            tip->set_text(text);
            tip->set_anchor(mx, my);
            tip->set_visible(true);
            Widget* root = win.root();
            if (root) root->append_child(tip);
            win.start_timer(tip, static_cast<u32>(kDelayMs));
            win.invalidate_all();
        }
    }
}

void TooltipManager::on_mouse_move(Window& win, f32 mx, f32 my) {
    mx_ = mx;
    my_ = my;
    auto it = tips_.find(&win);
    if (it == tips_.end() || !it->second->visible()) return;
    it->second->set_anchor(mx, my);
}

void TooltipManager::on_timer(Tooltip& tip) {
    Window* win = tip.window();
    if (!win || !tip.visible()) return;
    AnimationSystem& as = AnimationSystem::instance();
    as.tween(tip.progress_, 1.0f, kFadeMs, Easing::OutCubic,
             [&tip](f32 v) {
                 tip.progress_ = v;
                 tip.invalidate();
             });
}

void TooltipManager::hide(Window& win) {
    auto it = tips_.find(&win);
    if (it == tips_.end()) return;
    Tooltip* tip = it->second;
    if (!tip->visible()) return;
    win.stop_timer(tip);
    tip->set_visible(false);
    tip->remove_from_parent();
    win.invalidate_all();
}

}  // namespace yzk