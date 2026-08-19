#pragma once
#include <yuzuki/ui/widget.hpp>
#include <yuzuki/ui/animation.hpp>
#include <yuzuki/render/backend.hpp>
#include <yuzuki/controls/context_menu.hpp>

#include <vector>
#include <map>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace yzk {

class Window : public Widget {
public:
    explicit Window(const String& title, u32 width_dip, u32 height_dip);
    ~Window() override;

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool create(void* instance = nullptr);
    void destroy();
    void show();
    void close();
    void set_title(const String& title);

    bool is_window() const override { return true; }

    void* native_handle() const { return hwnd_; }
    bool is_created() const { return hwnd_ != nullptr; }
    bool needs_paint() const { return needs_paint_; }

    RenderBackend& backend() { return *backend_; }
    const RenderBackend& backend() const { return *backend_; }

    void set_root(Widget* widget);
    Widget* root() const { return root_; }

    void invalidate_area(const RectF& rect);
    void invalidate_all();
    void set_focus(Widget* widget);
    Widget* focused() const { return focused_; }
    // Renders one frame (record damage + replay + Present). Call once per drained
    // message batch; Present blocks on vsync, setting the render cadence.
    bool pump();

    void start_timer(Widget* widget, u32 interval_ms);
    void stop_timer(Widget* widget);

    void focus_next(bool reverse = false);
    void activate_focused();

    void set_context_menu(ContextMenu* menu) { context_menu_ = menu; }
    ContextMenu* context_menu() const { return context_menu_; }

    // Frame performance stats (cumulative; sample and diff over an interval for
    // averages): pump adds to these on every committed frame
    struct FrameStats {
        u64 frames = 0;     // frames committed
        f32 layout_ms = 0;  // layout time, cumulative
        f32 record_ms = 0;  // command-record time, cumulative
        f32 replay_ms = 0;  // command-replay time, cumulative
        f32 total_ms = 0;   // total frame time incl. Present, cumulative
        u64 commands = 0;   // recorded commands, cumulative
        u64 pump_calls = 0; // pump() calls (incl. skipped by frame-interval guard)
        u64 rendered_rects = 0; // damage rects rendered, cumulative
        u64 full_frames = 0;    // full-repaint frames, cumulative
    };
    const FrameStats& frame_stats() const { return frame_stats_; }

    // Borderless: hides the system title bar/frame; edges within 8 DIP resize
    // (WM_NCHITTEST) and maximized avoids the taskbar. Settable before or after create.
    void set_borderless(bool borderless);
    bool borderless() const { return borderless_; }

    // Custom caption: blank areas of the widget subtree return HTCAPTION (drag to
    // move); child controls still get mouse events. Does not own the pointer.
    void set_caption(Widget* widget);
    Widget* caption() const { return caption_; }

    void minimize();
    void maximize_toggle();
    bool maximized() const;

    // Unbinds window state pointers (hover/capture/focus/drag/timers) when a widget
    // is destroyed, so the window never dispatches to a dangling widget.
    void detach_widget(Widget* widget);

private:
    void kill_all_timers();
    void tick_animations();
    void refresh_animation_driver();
    void on_resize(u32 width_px, u32 height_px);
    void on_mouse_input(u32 message, f32 x_px, f32 y_px, u8 buttons, u8 mods);
    void on_wheel(f32 x_dip, f32 y_dip, i16 delta, u8 mods);
    void on_key(u32 message, u32 vk, u16 chr, u8 mods, bool repeat);
    bool caption_hit(f32 x, f32 y) const;
    // Borderless hit region (shared by WM_NCHITTEST / WM_SETCURSOR / manual drags):
    // returns an HT* resize zone or HTCLIENT (always HTCLIENT when maximized)
    int hit_zone(f32 x, f32 y) const;
    // Starts a manual drag/resize gesture: records window rect + cursor origin and
    // captures; WM_MOUSEMOVE drives SetWindowPos until button release
    void start_gesture(int zone);

    Widget* hit_test(f32 x, f32 y);
    void dispatch(Widget* target, Event& e);
    void update_hover(f32 x, f32 y);
    void update_cursor();
    // Incremental painted-extent convergence: diffs painted_bounds_ against the
    // committed baseline each frame and re-invalidates the delta
    void reconcile_painted_bounds(Widget* w, bool invalidate_delta);

    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    String title_;
    u32 client_width_ = 0;
    u32 client_height_ = 0;
    void* hwnd_ = nullptr;
    RenderBackend* backend_ = nullptr;

    Widget* root_ = nullptr;
    Widget* hover_ = nullptr;
    Widget* capture_ = nullptr;
    Widget* focused_ = nullptr;
    bool focus_visible_ = false;  // focus ring shows only after keyboard nav (Tab/Enter), not mouse clicks
    Widget* drag_source_ = nullptr;
    ContextMenu* context_menu_ = nullptr;
    f32 press_x_ = 0.0f;
    f32 press_y_ = 0.0f;
    f32 grab_dx_ = 0.0f;
    f32 grab_dy_ = 0.0f;
    bool press_armed_ = false;
    bool dragging_ = false;

    // Damage strategy: intersecting rects merge; over the cap the whole window
    // invalidates (damage_full_). Each damage rect is clipped and partially Presented.
    std::vector<RectF> damage_rects_;
    bool damage_full_ = false;
    bool needs_paint_ = false;
    bool closing_ = false;
    bool borderless_ = false;
    Widget* caption_ = nullptr;
    // Manual drag/resize state: resize_zone_ is HT* (resize) or HTCAPTION (move),
    // 0 = idle. Borderless windows drive gestures themselves — the system hit cache
    // reports HTCLIENT for client areas, so no WM_NCLBUTTONDOWN arrives.
    int resize_zone_ = 0;
    int resize_press_sx_ = 0;
    int resize_press_sy_ = 0;
    int resize_win_l_ = 0;
    int resize_win_t_ = 0;
    int resize_win_r_ = 0;
    int resize_win_b_ = 0;
    f32 last_cursor_pos_x_ = 0.0f;
    f32 last_cursor_pos_y_ = 0.0f;
    Cursor current_cursor_ = Cursor::Arrow;

    std::map<UINT_PTR, Widget*> timers_;
    UINT_PTR next_timer_id_ = 1;
    UINT_PTR anim_timer_id_ = 0;
    f32 last_anim_frame_ms_ = 0.0f;
    f32 last_frame_ms_ = 0.0f;
    FrameStats frame_stats_;
};

using WindowList = std::vector<Window*>;

}  // namespace yzk