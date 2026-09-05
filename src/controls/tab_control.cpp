#include <yuzuki/controls/tab_control.hpp>
#include <yuzuki/ui/paint.hpp>

#include <windows.h>

namespace yzk {

namespace {

constexpr f32 kHeaderPadH = 12.0f;   // horizontal padding per tab
constexpr f32 kCharWidth = 8.0f;     // estimate for header measure without a context
constexpr f32 kHeaderMinW = 56.0f;
constexpr f32 kIndicatorH = 2.0f;

Point to_local(Widget* widget, f32 x, f32 y) {
    const RectF g = widget->global_bounds();
    return Point{x - g.left, y - g.top};
}

f32 estimate_tab_width(const String& title, const PaintContext* ctx) {
    if (!ctx) return std::max(kHeaderMinW, static_cast<f32>(title.size()) * kCharWidth + kHeaderPadH * 2.0f);
    return std::max(kHeaderMinW, ctx->measure_text(title).width + kHeaderPadH * 2.0f);
}

}  // namespace

class TabControl::TabHeader : public Widget {
public:
    TabHeader(TabControl& owner, i32 index, String title)
        : owner_(owner), index_(index), title_(std::move(title)) {
        set_cursor(Cursor::Hand);
    }

    const String& title() const { return title_; }
    i32 tab_index() const { return index_; }
    void set_index(i32 index) { index_ = index; }

    Size measure_impl(Size available, const PaintContext* ctx) override {
        (void)available;
        const f32 h = ctx ? ctx->theme().control_height : 32.0f;
        return Size{estimate_tab_width(title_, ctx), h};
    }

    void paint_impl(PaintContext& ctx) override {
        const Theme& theme = ctx.theme();
        const bool active = index_ == owner_.selected_index();

        Color text_color = active ? theme.accent : theme.text_secondary;
        if (!active && has_flag(Flag_Hovered)) {
            ctx.fill_rounded(bounds_, theme.surface_container, theme.radius_sm);
            text_color = theme.text;
        }

        ctx.draw_text(title_, bounds_, text_color, TextAlignH::Center, TextAlignV::Center);

        if (active) {
            const f32 indent = kHeaderPadH * 0.5f;
            ctx.fill_rounded(RectF::make(bounds_.left + indent, bounds_.bottom - kIndicatorH,
                                         bounds_.width() - indent * 2.0f, kIndicatorH),
                             theme.accent, kIndicatorH * 0.5f);
        }
    }

    void on_event(Event& e) override {
        switch (e.type) {
            case EventType::MouseEnter:
                add_flag(Flag_Hovered);
                invalidate();
                e.consumed = true;
                break;

            case EventType::MouseLeave:
                remove_flag(Flag_Hovered);
                invalidate();
                e.consumed = true;
                break;

            case EventType::MouseDown:
                if (e.data.mouse.buttons & MouseButton_Left) {
                    const Point p = to_local(this, e.data.mouse.x, e.data.mouse.y);
                    if (p.x >= 0.0f && p.y >= 0.0f && p.x <= bounds_.width() && p.y <= bounds_.height()) {
                        owner_.request_focus();
                        owner_.set_selected_index(index_);
                        e.consumed = true;
                    }
                }
                break;

            case EventType::Click:
                e.consumed = true;
                break;

            default:
                break;
        }
    }

    String uia_name() const override { return title_; }
    String uia_role() const override { return "tab"; }

private:
    TabControl& owner_;
    i32 index_ = 0;
    String title_;
};

TabControl::~TabControl() {
    clear_tabs();
}

TabControl& TabControl::add_tab(String title, Widget* page) {
    auto* header = new TabHeader(*this, static_cast<i32>(headers_.size()), std::move(title));
    append_child(header);
    append_child(page);
    headers_.push_back(header);
    pages_.push_back(page);
    update_visibility();
    invalidate();
    return *this;
}

TabControl& TabControl::remove_tab(i32 index) {
    if (index < 0 || index >= tab_count()) return *this;
    if (selected_ == index) selected_ = 0;  // keep a valid selection; callers re-sync after
    delete headers_[static_cast<u32>(index)];
    delete pages_[static_cast<u32>(index)];
    headers_.erase(headers_.begin() + index);
    pages_.erase(pages_.begin() + index);
    if (selected_ > static_cast<i32>(pages_.size()) - 1 && !pages_.empty()) {
        selected_ = static_cast<i32>(pages_.size()) - 1;
    }
    for (i32 i = 0; i < tab_count(); ++i) headers_[static_cast<u32>(i)]->set_index(i);
    update_visibility();
    invalidate();
    return *this;
}

TabControl& TabControl::clear_tabs() {
    for (auto* h : headers_) delete h;
    for (auto* p : pages_) delete p;
    headers_.clear();
    pages_.clear();
    selected_ = 0;
    invalidate();
    return *this;
}

const String& TabControl::tab_title(i32 index) const {
    static const String kEmpty;
    if (index < 0 || index >= tab_count()) return kEmpty;
    return headers_[static_cast<u32>(index)]->title();
}

TabControl& TabControl::set_selected_index(i32 index) {
    if (index < 0 || index >= tab_count()) return *this;
    if (selected_ == index) return *this;
    selected_ = index;
    update_visibility();
    invalidate();
    if (window()) on_changed(index);
    return *this;
}

Widget* TabControl::selected_page() const {
    if (selected_ < 0 || selected_ >= tab_count()) return nullptr;
    return pages_[static_cast<u32>(selected_)];
}

Widget* TabControl::page(i32 index) const {
    if (index < 0 || index >= tab_count()) return nullptr;
    return pages_[static_cast<u32>(index)];
}

void TabControl::update_visibility() {
    for (i32 i = 0; i < tab_count(); ++i) {
        pages_[static_cast<u32>(i)]->set_visible(i == selected_);
    }
    for (auto* h : headers_) h->invalidate();
}

Size TabControl::measure_impl(Size available, const PaintContext* ctx) {
    const f32 header_h = ctx ? ctx->theme().control_height : 32.0f;
    f32 total_w = 0.0f;
    f32 content_h = 0.0f;
    for (i32 i = 0; i < tab_count(); ++i) {
        total_w += estimate_tab_width(tab_title(i), ctx);
        Widget* page = this->page(i);
        const Size s = page->measure(Size{available.width, available.height}, ctx);
        if (s.height > content_h) content_h = s.height;
    }
    if (available.width > total_w) total_w = available.width;  // fill horizontally when possible
    return Size{total_w, header_h + content_h};
}

void TabControl::perform_layout(const PaintContext* ctx) {
    Widget::perform_layout(ctx);
    const f32 header_h = ctx ? ctx->theme().control_height : 32.0f;

    f32 cursor = 0.0f;
    for (i32 i = 0; i < tab_count(); ++i) {
        const f32 w = estimate_tab_width(tab_title(i), ctx);
        headers_[static_cast<u32>(i)]->set_bounds(RectF::make(cursor, 0.0f, w, header_h));
        headers_[static_cast<u32>(i)]->perform_layout(ctx);
        cursor += w;
    }

    const RectF content = RectF::make(0.0f, header_h, bounds_.width(), bounds_.height() - header_h);
    for (i32 i = 0; i < tab_count(); ++i) {
        Widget* page = this->page(i);
        page->set_bounds(content);
        if (i == selected_) {
            page->perform_layout(ctx);
        }
    }
}

void TabControl::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const f32 header_h = ctx.theme().control_height;

    ctx.fill_rounded(RectF::make(bounds_.left, bounds_.top, bounds_.width(), header_h),
                     theme.surface, 0.0f);
    ctx.draw_line(Point{bounds_.left, bounds_.top + header_h},
                  Point{bounds_.right, bounds_.top + header_h}, theme.border, theme.border_width);

    Widget::paint_impl(ctx);
}

void TabControl::on_event(Event& e) {
    if (e.type == EventType::KeyDown && tab_count() > 0) {
        switch (e.data.key.code) {
            case VK_LEFT:
                set_selected_index((selected_ - 1 + tab_count()) % tab_count());
                e.consumed = true;
                return;
            case VK_RIGHT:
                set_selected_index((selected_ + 1) % tab_count());
                e.consumed = true;
                return;
            default:
                break;
        }
    }
    Widget::on_event(e);
}

}  // namespace yzk