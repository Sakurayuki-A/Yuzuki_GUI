#pragma once
#include <yuzuki/ui/widget.hpp>

#include <functional>
#include <vector>

namespace yzk {

// Tabbed multi-page container (5.4 Control Set): a header strip of tab buttons
// on top with the selected page filling the area below. Pages are owned by the
// control; only the active page is visible. Consumes theme tokens throughout
// (surface / border / accent / control_height), and answers to Left/Right arrows
// for keyboard navigation when focused.
class TabControl : public Widget {
public:
    TabControl() {
        set_focusable(true);
    }

    ~TabControl() override;

    // Page management. Both the header and the page widget are owned once appended.
    TabControl& add_tab(String title, Widget* page);

    // Removes a page and its header, deleting both.
    TabControl& remove_tab(i32 index);
    TabControl& clear_tabs();

    i32 tab_count() const { return static_cast<i32>(headers_.size()); }
    const String& tab_title(i32 index) const;

    i32 selected_index() const { return selected_; }
    TabControl& set_selected_index(i32 index);
    Widget* selected_page() const;
    Widget* page(i32 index) const;

    // Convenience callback registration; fired on selection change after the
    // control is attached to a Window (matches ComboBox semantics).
    TabControl& set_on_changed(std::function<void(i32)> cb) {
        on_changed_cb_ = std::move(cb);
        return *this;
    }
    // Fluent short name (Lego-style); same as set_on_changed.
    TabControl& on_changed(std::function<void(i32)> cb) { return set_on_changed(std::move(cb)); }
    virtual void on_changed(i32 index) {
        if (on_changed_cb_) on_changed_cb_(index);
    }

    Size measure_impl(Size available, const PaintContext* ctx) override;
    void perform_layout(const PaintContext* ctx = nullptr) override;
    void paint_impl(PaintContext& ctx) override;
    void on_event(Event& e) override;

    String uia_role() const override { return "tab control"; }

private:
    class TabHeader;
    void update_visibility();

    std::vector<TabHeader*> headers_;
    std::vector<Widget*> pages_;
    i32 selected_ = 0;
    std::function<void(i32)> on_changed_cb_;
};

}  // namespace yzk