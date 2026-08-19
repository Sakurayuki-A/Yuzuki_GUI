#pragma once
#include <yuzuki/core/types.hpp>
#include <yuzuki/ui/theme.hpp>

// Dynamic palette: colors follow the active Theme, so call sites need no changes on theme switch.
namespace codex {

class DynColor {
public:
    explicit constexpr DynColor(yzk::u8 role) : role_(role) {}
    operator yzk::Color() const {
        const yzk::Theme& t = yzk::Theme::get();
        switch (role_) {
            case 0: return t.background;
            case 1: return t.surface_container_low;
            case 2: return t.border;
            case 3: return t.text;
            case 4: return t.text_secondary;
            case 5: return t.surface_container_high;
            case 6: return t.surface_container;
            case 7: return t.surface_container_high;
            case 8: return t.border;
            case 9: return t.accent;
            case 10: return t.accent;
            case 11: return t.accent_hover;
            case 12: return t.accent_text;
            case 13: return t.accent;
            case 14: return t.accent;
            default: return t.text;
        }
    }

private:
    yzk::u8 role_;
};

inline constexpr DynColor WindowBg(0);
inline constexpr DynColor SidebarBg(1);
inline constexpr DynColor Border(2);
inline constexpr DynColor Text(3);
inline constexpr DynColor TextSecondary(4);
inline constexpr DynColor HoverBg(5);
inline constexpr DynColor CardBg(6);
inline constexpr DynColor CodeBg(7);
inline constexpr DynColor CodeBorder(8);
inline constexpr DynColor Success(9);
inline constexpr DynColor ButtonBg(10);
inline constexpr DynColor ButtonBgHover(11);
inline constexpr DynColor ButtonText(12);
inline constexpr DynColor UserBubbleBg(13);
inline constexpr DynColor Accent(14);

}  // namespace codex