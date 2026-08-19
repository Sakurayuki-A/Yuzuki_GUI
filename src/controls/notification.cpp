#include <yuzuki/controls/notification.hpp>
#include <yuzuki/controls/icon.hpp>
#include <yuzuki/ui/window.hpp>

namespace yzk {

namespace {

constexpr f32 kWidth = 340.0f;
constexpr f32 kMargin = 16.0f;
constexpr f32 kGap = 8.0f;
constexpr f32 kEnterMs = 220.0f;
constexpr f32 kExitMs = 180.0f;
constexpr f32 kSlidePx = 48.0f;

Color type_color(NotificationType type) {
    switch (type) {
        case NotificationType::Info:
            return Color::rgba(0x4A9EFF80);
        case NotificationType::Success:
            return Color::rgba(0x34C77B80);
        case NotificationType::Warning:
            return Color::rgba(0xF2B33D80);
        case NotificationType::Error:
            return Color::rgba(0xE5534B80);
    }
    return Color::rgba(0x4A9EFF80);
}

IconId type_icon(NotificationType type) {
    switch (type) {
        case NotificationType::Info:
            return IconId::Info;
        case NotificationType::Success:
            return IconId::Check;
        case NotificationType::Warning:
            return IconId::WarningCircle;
        case NotificationType::Error:
            return IconId::X;
    }
    return IconId::Info;
}

}  // namespace

// ===== Notification =====

Notification::Notification(String title, String message, NotificationType type)
    : title_(std::move(title)), message_(std::move(message)), type_(type) {
    set_cursor(Cursor::Hand);
}

Notification::~Notification() {
    if (Window* win = window()) win->stop_timer(this);
}

void Notification::wrap(const PaintContext* ctx, f32 max_width) {
    lines_.clear();
    if (max_width < 40.0f) max_width = 40.0f;
    String line;
    f32 line_w = 0.0f;
    String word;
    const char* p = message_.c_str();

    auto flush = [&]() {
        if (!line.empty()) lines_.push_back(line);
        line.clear();
        line_w = 0.0f;
    };
    auto push_word = [&]() {
        if (word.empty()) return;
        const f32 ww = ctx->measure_text(word, true).width;
        if (!line.empty() && line_w + 6.0f + ww > max_width) flush();
        if (!line.empty()) {
            line += ' ';
            line_w += 6.0f;
        }
        line += word;
        line_w += ww;
        word.clear();
    };

    while (*p) {
        if (*p == ' ' || *p == '\n') {
            push_word();
            if (*p == '\n') flush();
            ++p;
        } else {
            word += *p++;
        }
    }
    push_word();
    flush();
    if (lines_.empty()) lines_.push_back(String());
}

Size Notification::measure_impl(Size available, const PaintContext* ctx) {
    line_h_ = ctx->measure_text("Wg", true).height;
    if (!message_.empty()) {
        wrap(ctx, kWidth - 52.0f);
        last_height_ = 12.0f + 20.0f + (f32)lines_.size() * line_h_ + 12.0f;
    } else {
        last_height_ = 56.0f;
    }
    if (last_height_ < 56.0f) last_height_ = 56.0f;
    (void)available;
    return Size{kWidth, last_height_};
}

void Notification::paint_impl(PaintContext& ctx) {
    const f32 p = progress_;
    const Color accent = type_color(type_);
    const Theme& theme = ctx.theme();
    const Color card = theme.surface.with_alpha((u8)(theme.surface.a * p));
    const Color border = theme.border.with_alpha((u8)(theme.border.a * p));
    const Color text = theme.text.with_alpha((u8)(theme.text.a * p));
    const Color secondary = theme.text_secondary.with_alpha((u8)(theme.text_secondary.a * p));

    // Slide-in offset: from the right edge
    const f32 slide = (1.0f - ease(p, Easing::OutCubic)) * kSlidePx;
    const RectF rect = bounds_.translated(slide, 0.0f);

    ctx.draw_shadow(rect, 8.0f, 12.0f, Color{0x00, 0x00, 0x00, (u8)(40 * p)});
    ctx.fill_rounded(rect, card, 8.0f);
    ctx.draw_border(rect, border, 1.0f, 8.0f);

    const FontId icon_font = ctx.font(icon_family, 18.0f);
    const Color icon_color = accent.with_alpha((u8)(accent.a * p));
    ctx.draw_text(icon_font, icon_glyph(type_icon(type_)),
                  RectF::make(rect.left + 14.0f, rect.top + 10.0f, 20.0f, 20.0f), icon_color);

    ctx.draw_text(title_, RectF::make(rect.left + 40.0f, rect.top + 9.0f,
                                      rect.width() - 72.0f, 20.0f),
                  text, TextAlignH::Left, TextAlignV::Center);

    f32 body_y = rect.top + 33.0f;
    for (const String& line : lines_) {
        ctx.draw_text(line, RectF::make(rect.left + 40.0f, body_y, rect.width() - 52.0f, line_h_),
                      secondary, TextAlignH::Left, TextAlignV::Top);
        body_y += line_h_;
    }

    const Color close_color = (hovered_ ? text : secondary).with_alpha((u8)(secondary.a * p));
    ctx.draw_text(ctx.font(icon_family, 13.0f), icon_glyph(IconId::X),
                  RectF::make(rect.right - 28.0f, rect.top + 10.0f, 18.0f, 18.0f), close_color);
}

void Notification::on_event(Event& e) {
    switch (e.type) {
        case EventType::MouseEnter:
            hovered_ = true;
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseLeave:
            hovered_ = false;
            invalidate();
            e.consumed = true;
            break;

        case EventType::MouseUp:
            if (e.data.mouse.buttons == 0 || (e.data.mouse.buttons & MouseButton_Left)) {
                e.consumed = true;
                dismiss();
            }
            break;

        case EventType::Timer:
            e.consumed = true;
            dismiss();
            break;

        default:
            break;
    }
}

void Notification::perform_layout(const PaintContext* ctx) {
    if (ctx) measure(Size{kWidth, 1e7f}, ctx);
    if (Window* win = window()) NotificationManager::instance().layout_all(*win);
}

void Notification::begin_enter() {
    progress_ = 0.0f;
    active_tween_ = AnimationSystem::instance().tween(
        0.0f, 1.0f, kEnterMs, Easing::OutCubic,
        [this](f32 v) {
            progress_ = v;
            invalidate();
        });
    if (Window* win = window()) {
        win->start_timer(this, (u32)duration_ms_);
        win->invalidate_all();
    }
}

void Notification::dismiss() {
    if (dismissing_ || dismissed_) return;
    dismissing_ = true;
    // Token safety: the enter tween may have finished and been destroyed; stale tokens are ignored
    AnimationSystem::instance().finish_tween(active_tween_);
    if (Window* win = window()) win->stop_timer(this);
    active_tween_ = AnimationSystem::instance().tween(
        progress_, 0.0f, kExitMs, Easing::InCubic,
        [this](f32 v) {
            progress_ = v;
            invalidate();
            if (v <= 0.001f) finish();
        });
}

void Notification::finish() {
    if (dismissed_) return;
    dismissed_ = true;
    Window* win = window();
    if (win) {
        win->stop_timer(this);
        // Detach window state (hover/capture/focus/drag) before removal; otherwise
        // window() is null at destruction and stale hover pointers hit the deleted widget.
        win->detach_widget(this);
    }
    remove_from_parent();
    NotificationManager::instance().remove(this);
}

// ===== NotificationManager =====

NotificationManager& NotificationManager::instance() {
    static NotificationManager manager;
    return manager;
}

Notification& NotificationManager::show(Window& win, const String& title, const String& message,
                                        NotificationType type, f32 duration_ms) {
    auto* notification = new Notification(title, message, type);
    notification->set_duration_ms(duration_ms);
    notifications_.push_back(notification);
    Widget* root = win.root();
    if (root) root->append_child(notification);
    notification->begin_enter();
    return *notification;
}

void NotificationManager::dismiss_all(Window& win) {
    for (Notification* notification : notifications_) notification->dismiss();
    win.invalidate_all();
}

void NotificationManager::layout_all(Window& win) {
    const f32 width = win.bounds().width();
    const f32 height = win.bounds().height();
    f32 y = height - kMargin;
    for (Notification* notification : notifications_) {
        if (!notification->visible()) continue;
        const f32 h = notification->last_height() * notification->progress_;
        if (h <= 0.5f) continue;
        const f32 x = width - kMargin - kWidth;
        notification->set_bounds(RectF::make(x, y - h, kWidth, h));
        y -= h + kGap;
    }
}

void NotificationManager::remove(Notification* notification) {
    for (size_t i = 0; i < notifications_.size(); ++i) {
        if (notifications_[i] == notification) {
            Window* win = notification->window();
            notifications_.erase(notifications_.begin() + static_cast<ptrdiff_t>(i));
            delete notification;
            if (win) win->invalidate_all();
            return;
        }
    }
}

}  // namespace yzk