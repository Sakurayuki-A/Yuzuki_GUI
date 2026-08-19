#pragma once
#include <yuzuki/core/types.hpp>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace yzk {

enum class Easing : u8 {
    Linear,
    InQuad,
    OutQuad,
    InOutQuad,
    InCubic,
    OutCubic,
    InOutCubic,
    InBack,
    OutBack,
    InOutBack,
};

// Easing function: t in [0,1], returns eased progress
inline f32 ease(f32 t, Easing easing) {
    switch (easing) {
        case Easing::Linear:
            return t;
        case Easing::InQuad:
            return t * t;
        case Easing::OutQuad:
            return t * (2.0f - t);
        case Easing::InOutQuad:
            return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        case Easing::InCubic:
            return t * t * t;
        case Easing::OutCubic: {
            const f32 u = t - 1.0f;
            return u * u * u + 1.0f;
        }
        case Easing::InOutCubic:
            return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
        case Easing::InBack: {
            const f32 c = 1.70158f;
            return t * t * ((c + 1.0f) * t - c);
        }
        case Easing::OutBack: {
            const f32 c = 1.70158f;
            const f32 u = t - 1.0f;
            return u * u * ((c + 1.0f) * u + c) + 1.0f;
        }
        case Easing::InOutBack: {
            const f32 c = 1.70158f * 1.525f;
            if (t < 0.5f) {
                t *= 2.0f;
                return 0.5f * (t * t * ((c + 1.0f) * t - c));
            }
            t = t * 2.0f - 2.0f;
            return 0.5f * (t * t * ((c + 1.0f) * t + c) + 2.0f);
        }
    }
    return t;
}

// One tween: interpolates a value from -> to over time, calling on_update(value) each frame
class Tween {
public:
    using UpdateCallback = std::function<void(f32)>;

    Tween() = default;
    Tween(f32 from, f32 to, f32 duration_ms, Easing easing, UpdateCallback on_update)
        : from_(from), to_(to), duration_(duration_ms), easing_(easing), on_update_(std::move(on_update)) {}

    f32 value() const { return value_; }
    bool done() const { return done_; }
    bool running() const { return !done_ && elapsed_ < duration_; }

    void set_duration(f32 duration_ms) { duration_ = duration_ms; }
    void set_easing(Easing easing) { easing_ = easing; }
    void set_on_update(UpdateCallback cb) { on_update_ = std::move(cb); }

    // Jump straight to the end value
    void finish() {
        elapsed_ = duration_;
        advance(0.0f);
    }

    void advance(f32 dt_ms) {
        if (done_) return;
        elapsed_ += dt_ms;
        if (elapsed_ >= duration_) {
            value_ = to_;
            done_ = true;
        } else if (duration_ > 0.0f) {
            value_ = from_ + (to_ - from_) * ease(elapsed_ / duration_, easing_);
        }
        if (on_update_) on_update_(value_);
    }

private:
    f32 from_ = 0.0f;
    f32 to_ = 0.0f;
    f32 duration_ = 0.0f;
    f32 elapsed_ = 0.0f;
    Easing easing_ = Easing::Linear;
    f32 value_ = 0.0f;
    bool done_ = false;
    UpdateCallback on_update_;
};

// Animation system: global singleton owning all tweens and frame callbacks, driven per frame by windows
class AnimationSystem {
public:
    using Token = u64;
    using FrameToken = u64;
    static AnimationSystem& instance();

    // Starts a tween and returns a handle token. Tokens die with the tween — never
    // keep Tween raw pointers; finish_tween(token) is safe and stale tokens are ignored.
    Token tween(f32 from, f32 to, f32 duration_ms, Easing easing, Tween::UpdateCallback on_update);
    void finish_tween(Token token);
    void tick(f32 dt_ms);
    void stop_all();
    bool active() const { return !tweens_.empty(); }
    size_t count() const { return tweens_.size(); }

    // Per-render-frame callback (now_ms = real time in ms). Do not drive per-frame
    // animation with WM_TIMER: under message floods (frenzied mouse movement) WM_TIMER
    // delivery is delayed/coalesced and animation stutters. Frame callbacks follow the
    // render cadence (pump calls tick_frames per frame) and never starve.
    FrameToken on_frame(std::function<void(f32 now_ms)> cb) {
        FrameToken t = frame_next_token_++;
        frame_callbacks_[t] = std::move(cb);
        return t;
    }
    void stop_frame(FrameToken token) { frame_callbacks_.erase(token); }
    void tick_frames(f32 now_ms) {
        for (const auto& [_, cb] : frame_callbacks_) cb(now_ms);
    }
    size_t frame_count() const { return frame_callbacks_.size(); }

private:
    AnimationSystem() = default;
    std::vector<std::unique_ptr<Tween>> tweens_;
    std::vector<Token> tokens_;  // parallel to tweens_
    Token next_token_ = 1;
    std::map<FrameToken, std::function<void(f32)>> frame_callbacks_;
    FrameToken frame_next_token_ = 1;
};

// ===== Animatable properties =====

// Lerp trait: specialize for any type T. The default requires T to support
// a + (b - a) * t (numeric types like f32 work out of the box).
template <typename T>
struct PropLerp {
    static T lerp(const T& a, const T& b, f32 t) { return a + (b - a) * t; }
};

template <>
struct PropLerp<Color> {
    static Color lerp(const Color& a, const Color& b, f32 t) {
        return Color{static_cast<u8>(a.r + (b.r - a.r) * t),
                     static_cast<u8>(a.g + (b.g - a.g) * t),
                     static_cast<u8>(a.b + (b.b - a.b) * t),
                     static_cast<u8>(a.a + (b.a - a.a) * t)};
    }
};

template <>
struct PropLerp<Point> {
    static Point lerp(const Point& a, const Point& b, f32 t) {
        return Point{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
    }
};

template <>
struct PropLerp<RectF> {
    static RectF lerp(const RectF& a, const RectF& b, f32 t) {
        return RectF{a.left + (b.left - a.left) * t, a.top + (b.top - a.top) * t,
                     a.right + (b.right - a.right) * t, a.bottom + (b.bottom - a.bottom) * t};
    }
};

// Animatable property: value + tween. Usage:
//   AnimatableProperty<Color> fill{Color{...}};
//   fill.set_on_changed([this](const Color&) { invalidate(); });
//   fill.animate(newColor, 300ms, Easing::OutCubic);  // explicit tween
//   fill.set_transition(200); fill.set_animated(newColor);  // implicit transition (CSS transition)
class AnimatablePropertyBase {
public:
    // Kills the alive guard before cancelling: finish_tween fires one synchronous
    // callback, and the guard keeps it from touching already-destroyed members (UAF).
    ~AnimatablePropertyBase() {
        if (alive_) *alive_ = false;
        cancel();
    }

    void cancel() {
        if (token_) {
            AnimationSystem::instance().finish_tween(token_);
            token_ = 0;
        }
    }
    bool animating() const { return token_ != 0; }
    void set_transition(f32 ms) { transition_ms_ = ms; }
    f32 transition() const { return transition_ms_; }

protected:
    // Starts a tween driven uniformly by [0,1] progress; the callback interpolates per
    // type. It captures the alive guard so a destroyed property is never touched by
    // a tween still in the list.
    void start_impl(f32 duration_ms, Easing easing, std::function<void(f32)> on_update) {
        if (!alive_) alive_ = std::make_shared<bool>(true);
        AnimationSystem& as = AnimationSystem::instance();
        as.finish_tween(token_);
        token_ = as.tween(0.0f, 1.0f, duration_ms, easing,
                          [this, alive = alive_, on_update](f32 t) {
                              if (!*alive) return;
                              if (t >= 1.0f) token_ = 0;
                              on_update(t);
                          });
    }
    AnimationSystem::Token token_ = 0;
    f32 transition_ms_ = 0.0f;
    std::shared_ptr<bool> alive_;
};

template <typename T>
class AnimatableProperty : public AnimatablePropertyBase {
public:
    using ChangeCallback = std::function<void(const T&)>;

    AnimatableProperty() = default;
    explicit AnimatableProperty(T value) : value_(std::move(value)) {}

    const T& value() const { return value_; }
    operator const T&() const { return value_; }

    // Set immediately (no tween)
    void set(const T& v) {
        if (value_ == v) return;
        cancel();
        value_ = v;
        if (on_changed_) on_changed_(value_);
    }

    // Implicit transition: setters tween automatically once a transition duration is set
    void set_animated(const T& v) {
        if (value_ == v) return;
        if (transition_ms_ > 0.0f) {
            animate(v, transition_ms_, Easing::OutCubic);
        } else {
            set(v);
        }
    }

    void operator=(const T& v) { set_animated(v); }

    // Explicitly tween to a target value
    void animate(const T& to, f32 duration_ms, Easing easing = Easing::OutCubic) {
        if (value_ == to) return;
        if (duration_ms <= 0.0f) {
            set(to);
            return;
        }
        // Capture the current value BEFORE finishing the old tween: finishing first would
        // snap the value to the old target (visible jitter on rapid retriggers).
        const T from = value_;
        start_impl(duration_ms, easing, [this, from, to](f32 t) {
            value_ = PropLerp<T>::lerp(from, to, t);
            if (on_changed_) on_changed_(value_);
        });
    }

    void set_on_changed(ChangeCallback cb) { on_changed_ = std::move(cb); }

private:
    T value_{};
    ChangeCallback on_changed_;
};

}  // namespace yzk