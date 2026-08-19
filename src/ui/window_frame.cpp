// Window: render scheduling — damage merging / pump (layout→record→replay→Present) / animation driver / resizes.
#include <yuzuki/ui/window.hpp>

#include <yuzuki/ui/paint.hpp>

#include <chrono>

namespace yzk {

void Window::invalidate_area(const RectF& rect) {
    if (!hwnd_) return;
    // Damage-rect cap: beyond it, invalidate the whole window (many small rects usually
    // mean a large change). 8 is too low — rapid hover changes would then degrade to
    // full repaints; 32 small rects replay far cheaper than a full-window redraw.
    static constexpr size_t kMaxDamageRects = 32;
    RectF r = rect.intersect(
        RectF::make(0.0f, 0.0f, static_cast<f32>(client_width_), static_cast<f32>(client_height_)));
    if (r.empty()) return;
    // Union with backdrop-blur snapshot regions: snapshots read layer-accumulated
    // content, so a partial repaint would capture stale pixels outside the damage
    // rect (drag trails).
    if (backend_) {
        for (const RectF& region : backend_->backdrop_regions()) {
            if (region.intersects(r)) {
                r.unite(region);
                break;
            }
        }
    }
    r = r.intersect(
        RectF::make(0.0f, 0.0f, static_cast<f32>(client_width_), static_cast<f32>(client_height_)));
    if (r.empty()) return;
    needs_paint_ = true;
    if (damage_full_) return;
    // Bounded merging: intersecting rects merge, but an area cap prevents chain
    // inflation (merging adjacent cells would grow to full rows/window on hover
    // sweeps). Contained rects are ignored; over-cap rects stay separate — a little
    // overdraw beats a full-window repaint.
    static constexpr f32 kMaxMergedDamageArea = 4096.0f;  // ~64x64
    for (const RectF& d : damage_rects_) {
        if (d.contains_rect(r)) return;
    }
    for (RectF& d : damage_rects_) {
        if (!d.intersect(r).empty()) {
            RectF u = d;
            u.unite(r);
            if (u.width() * u.height() <= kMaxMergedDamageArea) {
                d = u;
                return;
            }
        }
    }
    // Too many rects = large-scale change: full repaint
    if (damage_rects_.size() >= kMaxDamageRects) {
        damage_full_ = true;
        damage_rects_.clear();
        return;
    }
    damage_rects_.push_back(r);
}

void Window::invalidate_all() {
    needs_paint_ = true;
    damage_full_ = true;
    damage_rects_.clear();
}

void Window::tick_animations() {
    AnimationSystem& system = AnimationSystem::instance();
    if (!system.active() && system.frame_count() == 0) return;
    const auto now = std::chrono::steady_clock::now();
    const f32 now_ms =
        static_cast<f32>(std::chrono::duration<double, std::milli>(now.time_since_epoch()).count());
    f32 dt = now_ms - last_anim_frame_ms_;
    last_anim_frame_ms_ = now_ms;
    if (dt < 0.0f) dt = 0.0f;
    if (dt > 50.0f) dt = 50.0f;
    system.tick(dt);
    // Frame callbacks must NOT run here: pump call frequency depends on message density
    // (hundreds/sec when input floods or Present doesn't block), which would speed up
    // incremental per-frame state. tick_frames runs once per actually rendered frame in pump.
}

void Window::refresh_animation_driver() {
    if (!hwnd_) return;
    AnimationSystem& system = AnimationSystem::instance();
    const bool active = system.active() || system.frame_count() > 0;
    if (active && anim_timer_id_ == 0) {
        anim_timer_id_ = next_timer_id_++;
        timers_[anim_timer_id_] = this;
        // 1ms interval (with timeBeginPeriod(1) high-precision clock); the real animation
        // frame rate is set by Present vsync
        SetTimer(static_cast<HWND>(hwnd_), anim_timer_id_, 1, nullptr);
    } else if (!active && anim_timer_id_ != 0) {
        KillTimer(static_cast<HWND>(hwnd_), anim_timer_id_);
        timers_.erase(anim_timer_id_);
        anim_timer_id_ = 0;
    }
}

void Window::reconcile_painted_bounds(Widget* w, bool invalidate_delta) {
    if (!w) return;
    for (Widget* child = w->first_child(); child; child = child->next_sibling()) {
        reconcile_painted_bounds(child, invalidate_delta);
    }
    const RectF pb = w->painted_bounds_;
    if (pb != w->painted_bounds_committed_) {
        if (invalidate_delta) {
            RectF r = pb;
            r.unite(w->painted_bounds_committed_);
            invalidate_area(r);
        }
        w->painted_bounds_committed_ = pb;
    }
}

bool Window::pump() {
    if (!hwnd_ || !backend_) return false;
    tick_animations();
    refresh_animation_driver();
    ++frame_stats_.pump_calls;
    if (!needs_paint_) return false;

    // Render cadence = display refresh: Application::run calls pump once per drained
    // message batch and Present blocks on vsync. The cap below is a safety net:
    // when occluded/minimized Present returns immediately (DXGI_STATUS_OCCLUDED
    // doesn't block), so without it rendering would spin the CPU at full speed.
    static constexpr f32 kMinFrameIntervalMs = 16.0f;
    const auto now = std::chrono::steady_clock::now();
    const f32 now_ms =
        static_cast<f32>(std::chrono::duration<double, std::milli>(now.time_since_epoch()).count());
    if (now_ms - last_frame_ms_ < kMinFrameIntervalMs) {
        return false;  // keep needs_paint_ = true so damage keeps accumulating
    }
    last_frame_ms_ = now_ms;
    needs_paint_ = false;

    // Frame callbacks run per rendered frame, sampling real time for constant speed;
    // their invalidations set needs_paint_ and drive the next frame (continuous animation)
    AnimationSystem::instance().tick_frames(now_ms);

    const bool full = damage_full_;
    std::vector<RectF> damage;
    if (full) {
        damage.clear();
    } else {
        damage = std::move(damage_rects_);
    }
    damage_rects_.clear();
    damage_full_ = false;

    const auto t_frame0 = std::chrono::steady_clock::now();
    const auto t_layout0 = t_frame0;
    auto t_layout1 = t_frame0;
    auto t_record1 = t_frame0;

    const RectF client = RectF::make(0.0f, 0.0f, static_cast<f32>(client_width_), static_cast<f32>(client_height_));
    if (!full) {
        for (auto it = damage.begin(); it != damage.end();) {
            if (it->intersect(client).empty()) {
                it = damage.erase(it);
            } else {
                ++it;
            }
        }
        if (damage.empty()) return false;
    }

    // On failure/abort, restore the damage rects for a retry next frame
    const auto restore = [&] {
        needs_paint_ = true;
        damage_full_ = full;
        damage_rects_ = damage;
    };

    for (int attempt = 0; attempt < 3; ++attempt) {
        const bool ok = full ? backend_->begin_frame(Theme::get().background, nullptr)
                             : backend_->begin_partial_frame(Theme::get().background);
        if (!ok) {
            restore();
            return false;
        }
        PaintContext ctx(*backend_, Theme::get());
        if (root_) {
            ++g_layout_pass;
            root_->set_bounds(bounds_);
            root_->perform_layout(&ctx);
            t_layout1 = std::chrono::steady_clock::now();
            // Record the command list: the tree is traversed once per frame (with widget-level
            // culling for partial frames), then replayed per damage rect
            ctx.begin_record(full ? nullptr : &damage);
            root_->paint(ctx);
            if (focus_visible_ && focused_ && focused_->visible() && focused_->focusable()) {
                ctx.set_offset(0.0f, 0.0f);
                const f32 r = Theme::get().corner_radius + 2.0f;
                const RectF gb = focused_->global_bounds();
                ctx.draw_border(gb.inflated(2.0f, 2.0f), Theme::get().text, 2.0f, r);
            }
            ctx.end_record();
            t_record1 = std::chrono::steady_clock::now();
            if (full) {
                ctx.replay(RectF{});
            } else {
                for (const RectF& r : damage) {
                    if (!backend_->begin_damage_rect(r)) break;
                    ctx.replay(r);
                    backend_->end_damage_rect();
                }
            }
        }
        if (backend_->end_frame()) {
            const auto t_frame1 = std::chrono::steady_clock::now();
            const auto ms = [](auto from, auto to) {
                return static_cast<f32>(
                    std::chrono::duration<double, std::milli>(to - from).count());
            };
            FrameStats& st = frame_stats_;
            ++st.frames;
            st.total_ms += ms(t_frame0, t_frame1);
            st.commands += ctx.command_count();
            st.rendered_rects += static_cast<u64>(damage.size());
            st.full_frames += full ? 1 : 0;
            if (root_) {
                st.layout_ms += ms(t_layout0, t_layout1);
                st.record_ms += ms(t_layout1, t_record1);
                st.replay_ms += ms(t_record1, t_frame1);
            }
            // Incremental painted-extent convergence: re-invalidates spill changes (e.g. growing hover shadows) for the next frame
            reconcile_painted_bounds(root_, !full);
            // Deferred shadow textures were generated this frame: repaint now so shadows appear immediately
            if (backend_->shadows_pending_after_frame()) {
                needs_paint_ = true;
                damage_rects_ = damage;
                damage_full_ = full;
            }
            return true;
        }
        restore();
    }
    return false;
}

void Window::on_resize(u32 width_px, u32 height_px) {
    const f32 scale = backend_ ? backend_->dpi_scale() : 1.0f;
    client_width_ = static_cast<u32>(static_cast<f32>(width_px) / scale + 0.5f);
    client_height_ = static_cast<u32>(static_cast<f32>(height_px) / scale + 0.5f);
    bounds_ = RectF::make(0.0f, 0.0f, static_cast<f32>(client_width_), static_cast<f32>(client_height_));
    if (backend_) backend_->resize(width_px, height_px);
    invalidate_all();
}

}  // namespace yzk