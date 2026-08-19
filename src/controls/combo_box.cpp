#include <yuzuki/controls/combo_box.hpp>
#include <yuzuki/controls/list_view.hpp>
#include <yuzuki/controls/scroll_view.hpp>
#include <yuzuki/ui/window.hpp>

#include <windows.h>

namespace yzk {

namespace {

constexpr f32 kRowHeight = 24.0f;
constexpr i32 kMaxRows = 6;

Point to_local(Widget* widget, f32 x, f32 y) {
    const RectF g = widget->global_bounds();
    return Point{x - g.left, y - g.top};
}

}  // namespace

class ComboBox::Popup : public Overlay {
public:
    explicit Popup(ComboBox& owner) : owner_(owner) {
        set_dim(Color{0x00, 0x00, 0x00, 0x00});
        set_animated(false);  // Popup appears/disappears instantly, no slide animation
        list_ = new PopupList(*this);
        list_->set_row_height(kRowHeight);
        list_->set_show_border(false);
        scroll_ = new ScrollView();
        scroll_->set_content(list_);
        append_child(scroll_);
    }

    ~Popup() override {
        scroll_->remove_from_parent();
        delete scroll_;
        delete list_;
    }

    void sync_items() {
        list_->set_items(owner_.items_);
        list_->set_selected(owner_.selected_);
        hover_index_ = owner_.selected_;
        if (hover_index_ < 0 && !owner_.items_.empty()) hover_index_ = 0;
        list_->set_hovered(hover_index_);
        scroll_->set_scroll_y(0.0f);
    }

protected:
    void perform_layout(const PaintContext* ctx = nullptr) override {
        Overlay::perform_layout(ctx);
        scroll_->set_bounds(panel_rect());
        scroll_->perform_layout(ctx);
    }

    void paint_impl(PaintContext& ctx) override {
        Overlay::paint_impl(ctx);
    }

    void on_event(Event& e) override {
        switch (e.type) {
            case EventType::KeyDown: {
                const i32 count = static_cast<i32>(owner_.items_.size());
                if (count == 0) break;
                switch (e.data.key.code) {
                    case VK_UP:
                    case VK_DOWN: {
                        if (hover_index_ < 0) hover_index_ = owner_.selected_;
                        if (hover_index_ < 0) hover_index_ = 0;
                        if (e.data.key.code == VK_UP) {
                            --hover_index_;
                            if (hover_index_ < 0) hover_index_ = count - 1;
                        } else {
                            ++hover_index_;
                            if (hover_index_ >= count) hover_index_ = 0;
                        }
                        list_->set_hovered(hover_index_);
                        scroll_to_visible();
                        e.consumed = true;
                        return;
                    }
                    case VK_RETURN:
                        if (hover_index_ >= 0 && hover_index_ < count) {
                            e.consumed = true;
                            list_->set_selected(hover_index_);
                            return;
                        }
                        break;
                    default:
                        break;
                }
                break;
            }

            default:
                break;
        }
        Overlay::on_event(e);
    }

private:
    class PopupList : public ListView {
    public:
        explicit PopupList(Popup& owner) : owner_(owner) {}

        void on_selected(i32 index) override {
            owner_.owner_.set_selected_index(index);
            owner_.owner_.close_popup();
        }

    private:
        Popup& owner_;
    };

    void scroll_to_visible() {
        const f32 view_h = scroll_->bounds().height();
        if (view_h <= 0.0f) return;
        const f32 row_top = static_cast<f32>(hover_index_) * kRowHeight;
        const f32 sy = scroll_->scroll_y();
        if (row_top < sy) {
            scroll_->set_scroll_y(row_top);
        } else if (row_top + kRowHeight > sy + view_h) {
            scroll_->set_scroll_y(row_top + kRowHeight - view_h);
        }
    }

    ComboBox& owner_;
    ScrollView* scroll_ = nullptr;
    PopupList* list_ = nullptr;
    i32 hover_index_ = -1;
};

ComboBox::ComboBox() {
    set_cursor(Cursor::Hand);
    set_focusable(true);
}

ComboBox::ComboBox(std::vector<String> items) : ComboBox() {
    set_items(std::move(items));
}

ComboBox::~ComboBox() {
    if (popup_) {
        popup_->close();
        delete popup_;
        popup_ = nullptr;
    }
}

void ComboBox::set_items(std::vector<String> items) {
    items_ = std::move(items);
    if (selected_ >= static_cast<i32>(items_.size())) {
        selected_ = -1;
    }
    invalidate();
}

void ComboBox::clear_items() {
    items_.clear();
    selected_ = -1;
    invalidate();
}

void ComboBox::set_selected_index(i32 index) {
    if (index < -1 || index >= static_cast<i32>(items_.size())) return;
    if (selected_ == index) return;
    selected_ = index;
    invalidate();
    if (index >= 0) on_change(index);
}

const String& ComboBox::selected_text() const {
    static const String kEmpty;
    if (selected_ < 0 || selected_ >= static_cast<i32>(items_.size())) return kEmpty;
    return items_[static_cast<u32>(selected_)];
}

bool ComboBox::is_open() const {
    return popup_ && popup_->is_open();
}

void ComboBox::open_popup() {
    Window* win = window();
    if (!win || is_open() || items_.empty()) return;

    const RectF g = global_bounds();
    const u32 rows = std::min<u32>(static_cast<u32>(items_.size()), static_cast<u32>(kMaxRows));
    const f32 list_h = static_cast<f32>(rows) * kRowHeight;
    const f32 gap = 8.0f;

    f32 x = g.left;
    f32 y = g.bottom + gap;
    if (y + list_h > win->bounds().bottom && g.top - gap - list_h >= win->bounds().top) {
        y = g.top - gap - list_h;
    }

    if (!popup_) popup_ = new Popup(*this);
    popup_->sync_items();
    popup_->set_panel_rect(RectF::make(x, y, g.width(), list_h));
    popup_->show(*win);
    invalidate();
}

void ComboBox::close_popup() {
    if (popup_ && popup_->is_open()) {
        popup_->close();
        invalidate();
    }
}

Size ComboBox::measure_impl(Size available, const PaintContext* ctx) {
    (void)available;
    (void)ctx;
    return Size{width_, 32.0f};
}

void ComboBox::paint_impl(PaintContext& ctx) {
    const Theme& theme = ctx.theme();
    const f32 radius = theme.corner_radius;

    Color border = is_open() ? theme.accent : theme.border;
    if (has_flag(Flag_Hovered) && !is_open()) border = theme.border_hover;

    ctx.fill_rounded(bounds_, theme.surface, radius);
    ctx.draw_border(bounds_, border, 1.0f, radius);

    const RectF text_rect = RectF::make(bounds_.left + 10.0f, bounds_.top,
                                        bounds_.width() - 10.0f - 26.0f, bounds_.height());
    if (selected_ >= 0 && selected_ < static_cast<i32>(items_.size())) {
        ctx.draw_text(items_[static_cast<u32>(selected_)], text_rect, theme.text,
                      TextAlignH::Left, TextAlignV::Center);
    } else {
        ctx.draw_text(placeholder_, text_rect, theme.text_disabled,
                      TextAlignH::Left, TextAlignV::Center);
    }

    const Point c{bounds_.right - 15.0f, bounds_.top + bounds_.height() * 0.5f};
    const Color arrow = theme.text_secondary;
    ctx.draw_line(Point{c.x - 3.5f, c.y - 1.5f}, Point{c.x, c.y + 2.0f}, arrow, 1.5f);
    ctx.draw_line(Point{c.x, c.y + 2.0f}, Point{c.x + 3.5f, c.y - 1.5f}, arrow, 1.5f);
}

void ComboBox::on_event(Event& e) {
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
                    if (is_open()) {
                        close_popup();
                    } else {
                        open_popup();
                    }
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

}  // namespace yzk