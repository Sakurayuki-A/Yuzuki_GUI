#include <yuzuki/ui/dialog.hpp>
#include <yuzuki/ui/window.hpp>
#include <yuzuki/core/encoding.hpp>

#include <windows.h>
#include <shobjidl.h>

#undef MessageBox  // winuser.h maps MessageBox → MessageBoxW; our class keeps the name

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace yzk {

class MessageBoxPanel;
class MessageBoxPanelHolder;

struct MessageBox::Impl {
    MessageBoxPanel* panel = nullptr;
    Window* win = nullptr;
    std::function<void(MessageBoxResult)> on_result;
    MessageBoxResult pending = MessageBoxResult::None;
};

namespace {

constexpr f32 kPanelWidth = 380.0f;
constexpr f32 kPanelPadding = 22.0f;
constexpr f32 kTitleH = 20.0f;
constexpr f32 kTextGap = 12.0f;
constexpr f32 kButtonH = 32.0f;
constexpr f32 kButtonGap = 10.0f;
constexpr f32 kMinButtonW = 88.0f;

f32 measure_text_height(const PaintContext* ctx, const String& text, f32 max_width) {
    const Size s = ctx->measure_text(text, true, max_width, true);
    return s.height;
}

}  // namespace

// Message body drawn with word-wrap (Label is single-line only, so long messages
// would overflow the panel). Mirrors the chat Bubble pattern: measure with wrap
// in the panel, paint with wrap=true here.
class MessageText : public Widget {
public:
    void set_text(const String& text) {
        text_ = text;
        invalidate();
    }

    const String& text() const { return text_; }

protected:
    void paint_impl(PaintContext& ctx) override {
        const String& t = text_.empty() ? SingleSpace : text_;
        ctx.draw_text_small(t, bounds_, ctx.theme().text_secondary, TextAlignH::Left,
                            TextAlignV::Top, true);
    }

private:
    static constexpr const char* SingleSpace = " ";
    String text_;
};

class MessageBoxPanel : public Overlay {
public:
    explicit MessageBoxPanel(MessageBox::Impl& impl) : impl_(&impl) {
        title_ = new Label("");
        title_->set_bold(true);
        message_ = new MessageText;
        append_child(title_);
        append_child(message_);
    }

    ~MessageBoxPanel() override {
        title_->remove_from_parent();
        delete title_;
        message_->remove_from_parent();
        delete message_;
        clear_buttons();
    }

    // A MessageBox instance may be destroyed while its panel is still on screen
    // (e.g. a stack MessageBox in a click handler). Self-owning keeps the Impl
    // (and thus the panel) alive until the close animation finishes, then frees
    // both — no dangling tween lambda.
    void self_own() { self_owned_ = true; }

    void on_close() override {
        if (self_owned_) {
            self_owned_ = false;
            MessageBox::Impl* gone = impl_;
            impl_ = nullptr;
            delete gone;
            delete this;
        }
    }

    // Single dismissal path: records the pending result, starts the close
    // animation, and fires the user callback exactly once (synchronously).
    void dismiss_with(MessageBoxResult result) {
        if (!is_open()) return;
        impl_->pending = result;
        close();
        if (impl_->on_result) impl_->on_result(result);
    }

    void set_content(const String& title, const String& message) {
        title_->set_text(title);
        message_->set_text(message);
    }

    void set_buttons(MessageBoxKind kind) {
        clear_buttons();
        switch (kind) {
            case MessageBoxKind::Confirm:
                push_button("OK", MessageBoxResult::Ok);
                push_button("Cancel", MessageBoxResult::Cancel);
                break;
            case MessageBoxKind::YesNo:
                push_button("Yes", MessageBoxResult::Yes);
                push_button("No", MessageBoxResult::No);
                break;
            case MessageBoxKind::YesNoCancel:
                push_button("Yes", MessageBoxResult::Yes);
                push_button("No", MessageBoxResult::No);
                push_button("Cancel", MessageBoxResult::Cancel);
                break;
            case MessageBoxKind::Info:
            default:
                push_button("OK", MessageBoxResult::Ok);
                break;
        }
    }

protected:
    void perform_layout(const PaintContext* ctx = nullptr) override {
        Overlay::perform_layout(ctx);
        const RectF win = bounds();

        const f32 text_w = kPanelWidth - kPanelPadding * 2.0f;
        const String msg = message_->text();
        const f32 msg_h = ctx && !msg.empty()
                              ? measure_text_height(ctx, msg, text_w)
                              : kTitleH;
        const f32 panel_h = kPanelPadding + kTitleH + kTextGap + msg_h + kTextGap +
                            kButtonH + kPanelPadding;

        const RectF panel = RectF::make(
            win.left + (win.width() - kPanelWidth) * 0.5f,
            win.top + (win.height() - panel_h) * 0.5f, kPanelWidth, panel_h);
        set_panel_rect(panel);

        // Children bounds are relative to this widget (panel spans window), so
        // every element must be offset by the centered panel's origin.
        const f32 pl = panel.left;
        const f32 pt = panel.top;
        title_->set_bounds(RectF::make(pl + kPanelPadding, pt + kPanelPadding, text_w, kTitleH));
        message_->set_bounds(RectF::make(pl + kPanelPadding, pt + kPanelPadding + kTitleH + kTextGap,
                                         text_w, msg_h));

        // Buttons right-aligned on the bottom row (relative to panel).
        const i32 n = button_count();
        f32 total_w = 0.0f;
        for (i32 i = 0; i < n; ++i) total_w += button_width(i) + kButtonGap;
        total_w = total_w > 0.0f ? total_w - kButtonGap : 0.0f;
        f32 bx = pl + kPanelWidth - kPanelPadding - total_w;
        const f32 by = pt + panel_h - kPanelPadding - kButtonH;
        for (i32 i = 0; i < n; ++i) {
            button_at(i)->set_bounds(RectF::make(bx, by, button_width(i), kButtonH));
            bx += button_width(i) + kButtonGap;
        }

        for (Widget* child = first_child(); child; child = child->next_sibling()) {
            child->perform_layout(ctx);
        }
    }

    void on_event(Event& e) override {
        if (e.type == EventType::KeyDown && e.data.key.code == VK_RETURN && button_count() > 0) {
            // Enter activates the default (first) button.
            e.consumed = true;
            dismiss_with(button_result(0));
            return;
        }
        if (e.type == EventType::KeyDown && e.data.key.code == VK_ESCAPE) {
            e.consumed = true;
            dismiss_with(has_cancel_button() ? MessageBoxResult::Cancel : MessageBoxResult::None);
            return;
        }
        if (e.type == EventType::MouseDown && (e.data.mouse.buttons & MouseButton_Left)) {
            const RectF g = global_bounds();
            const f32 lx = e.data.mouse.x - g.left;
            const f32 ly = e.data.mouse.y - g.top;
            if (!panel_rect().contains(lx, ly)) {
                e.consumed = true;
                dismiss_with(MessageBoxResult::None);
                return;
            }
        }
        Overlay::on_event(e);
    }

private:
    struct ButtonSpec {
        Button* widget = nullptr;
        MessageBoxResult result = MessageBoxResult::None;
    };

    void clear_buttons() {
        for (Widget* child = first_child(); child;) {
            Widget* next = child->next_sibling();
            if (child != title_ && child != message_) {
                child->remove_from_parent();
                delete child;
            }
            child = next;
        }
        buttons_.clear();
    }

    void push_button(const String& text, MessageBoxResult result) {
        auto* btn = new Button(text);
        btn->set_on_click([this, result]() { dismiss_with(result); });
        append_child(btn);
        buttons_.push_back(ButtonSpec{btn, result});
        invalidate();
    }

    i32 button_count() const { return static_cast<i32>(buttons_.size()); }
    Widget* button_at(i32 i) const { return buttons_[static_cast<u32>(i)].widget; }
    MessageBoxResult button_result(i32 i) const {
        return buttons_[static_cast<u32>(i)].result;
    }
    f32 button_width(i32 i) const {
        return std::max(kMinButtonW, button_at(i)->min_size().width);
    }
    bool has_cancel_button() const {
        for (const auto& b : buttons_)
            if (b.result == MessageBoxResult::Cancel) return true;
        return false;
    }

    MessageBox::Impl* impl_ = nullptr;
    bool self_owned_ = false;
    Label* title_ = nullptr;
    MessageText* message_ = nullptr;
    std::vector<ButtonSpec> buttons_;
};

MessageBox::MessageBox() : impl_(new Impl) {
    impl_->panel = new MessageBoxPanel(*impl_);
}

MessageBox::~MessageBox() {
    if (impl_ && impl_->panel->is_open()) {
        // Keep the modal alive after this MessageBox instance dies (the typical
        // fire-and-forget pattern in a click handler). The panel now owns impl_.
        impl_->panel->self_own();
        return;
    }
    close();
    delete impl_->panel;
    delete impl_;
}

void MessageBox::show(Window& win, const String& title, const String& message,
                      MessageBoxKind kind,
                      std::function<void(MessageBoxResult)> on_result) {
    if (impl_->panel->is_open()) return;
    impl_->win = &win;
    impl_->on_result = std::move(on_result);
    impl_->pending = MessageBoxResult::None;
    impl_->panel->set_content(title, message);
    impl_->panel->set_buttons(kind);
    impl_->panel->show(win);
}

void MessageBox::close() {
    if (!impl_->panel->is_open()) return;
    impl_->panel->dismiss_with(MessageBoxResult::None);
}

bool MessageBox::is_open() const { return impl_->panel->is_open(); }

void MessageBox::set_animated(bool animated) {
    impl_->panel->set_animated(animated);
}

void MessageBox::resolve(MessageBoxResult result) {
    if (impl_->panel->is_open()) impl_->panel->dismiss_with(result);
}

// ===== Native file dialogs =====

namespace {

void split_filters(const std::vector<FileDialogFilter>& filters,
                   std::vector<COMDLG_FILTERSPEC>& specs,
                   std::vector<std::wstring>& names,
                   std::vector<std::wstring>& patterns) {
    names.reserve(filters.size());
    patterns.reserve(filters.size());
    for (const auto& f : filters) {
        names.push_back(utf::to_wide(f.name));
        patterns.push_back(utf::to_wide(f.pattern));
    }
    specs.resize(names.size());
    for (size_t i = 0; i < names.size(); ++i) {
        specs[i].pszName = names[i].c_str();
        specs[i].pszSpec = patterns[i].c_str();
    }
}

String item_path(IShellItem* item) {
    if (!item) return String();
    LPWSTR raw = nullptr;
    if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &raw))) return String();
    const String path = utf::to_utf8(raw ? raw : L"");
    CoTaskMemFree(raw);
    return path;
}

bool ensure_com() {
    if (SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) return true;
    return false;  // already initialized on this thread; don't uninitialize at the end
}

}  // namespace

std::vector<String> open_file_dialog(Window* owner, const FileDialogOptions& opts) {
    const bool com_owned = ensure_com();
    IFileOpenDialog* dlg = nullptr;
    const HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_PPV_ARGS(&dlg));
    if (FAILED(hr)) {
        if (com_owned) CoUninitialize();
        return {};
    }

    DWORD flags = FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST;
    if (opts.multi_select) flags |= FOS_ALLOWMULTISELECT;
    dlg->SetOptions(flags);

    std::vector<COMDLG_FILTERSPEC> specs;
    std::vector<std::wstring> names, patterns;
    if (!opts.filters.empty()) {
        split_filters(opts.filters, specs, names, patterns);
        dlg->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
    }
    if (!opts.default_name.empty()) {
        dlg->SetFileName(utf::to_wide(opts.default_name).c_str());
    }

    std::vector<String> out;
    if (SUCCEEDED(dlg->Show(owner ? static_cast<HWND>(owner->native_handle()) : nullptr))) {
        IShellItemArray* items = nullptr;
        if (SUCCEEDED(dlg->GetResults(&items)) && items) {
            DWORD count = 0;
            items->GetCount(&count);
            for (DWORD i = 0; i < count; ++i) {
                IShellItem* item = nullptr;
                if (SUCCEEDED(items->GetItemAt(i, &item)) && item) {
                    const String p = item_path(item);
                    if (!p.empty()) out.push_back(p);
                    item->Release();
                }
            }
            items->Release();
        }
    }
    dlg->Release();
    if (com_owned) CoUninitialize();
    return out;
}

String save_file_dialog(Window* owner, const FileDialogOptions& opts) {
    const bool com_owned = ensure_com();
    IFileSaveDialog* dlg = nullptr;
    const HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_PPV_ARGS(&dlg));
    if (FAILED(hr)) {
        if (com_owned) CoUninitialize();
        return String();
    }

    dlg->SetOptions(FOS_FORCEFILESYSTEM | FOS_OVERWRITEPROMPT);

    std::vector<COMDLG_FILTERSPEC> specs;
    std::vector<std::wstring> names, patterns;
    if (!opts.filters.empty()) {
        split_filters(opts.filters, specs, names, patterns);
        dlg->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
    }
    if (!opts.default_name.empty()) {
        dlg->SetFileName(utf::to_wide(opts.default_name).c_str());
    }

    String out;
    if (SUCCEEDED(dlg->Show(owner ? static_cast<HWND>(owner->native_handle()) : nullptr))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item)) && item) {
            out = item_path(item);
            item->Release();
        }
    }
    dlg->Release();
    if (com_owned) CoUninitialize();
    return out;
}

}  // namespace yzk