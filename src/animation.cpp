#include <yuzuki/ui/animation.hpp>

namespace yzk {

AnimationSystem& AnimationSystem::instance() {
    static AnimationSystem system;
    return system;
}

AnimationSystem::Token AnimationSystem::tween(f32 from, f32 to, f32 duration_ms, Easing easing,
                                               Tween::UpdateCallback on_update) {
    tweens_.push_back(std::make_unique<Tween>(from, to, duration_ms, easing, std::move(on_update)));
    tokens_.push_back(next_token_);
    return next_token_++;
}

void AnimationSystem::finish_tween(Token token) {
    for (size_t i = 0; i < tweens_.size(); ++i) {
        if (tokens_[i] == token) {
            Tween* tween = tweens_[i].get();
            tween->finish();
            return;
        }
    }
}

void AnimationSystem::tick(f32 dt_ms) {
    if (tweens_.empty()) return;
    for (size_t i = 0; i < tweens_.size();) {
        Tween* tween = tweens_[i].get();
        tween->advance(dt_ms);
        if (tween->done()) {
            tweens_.erase(tweens_.begin() + static_cast<ptrdiff_t>(i));
            tokens_.erase(tokens_.begin() + static_cast<ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
}

void AnimationSystem::stop_all() {
    tweens_.clear();
}

}  // namespace yzk