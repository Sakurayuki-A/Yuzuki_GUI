#include <yuzuki/ui/theme.hpp>
#include <yuzuki/ui/application.hpp>

namespace yzk {

namespace {
Theme g_theme = Theme::make_dark();
}

const Theme& Theme::get() {
    return g_theme;
}

const Color& theme_color(const Theme& theme, ThemeRole role) {
    switch (role) {
        case ThemeRole::Background: return theme.background;
        case ThemeRole::Surface: return theme.surface;
        case ThemeRole::SurfaceContainerLow: return theme.surface_container_low;
        case ThemeRole::SurfaceContainer: return theme.surface_container;
        case ThemeRole::SurfaceContainerHigh: return theme.surface_container_high;
        case ThemeRole::Border: return theme.border;
        case ThemeRole::Track: return theme.track;
        case ThemeRole::Accent: return theme.accent;
        case ThemeRole::AccentText: return theme.accent_text;
        case ThemeRole::Text: return theme.text;
        case ThemeRole::TextSecondary: return theme.text_secondary;
        case ThemeRole::TextDisabled: return theme.text_disabled;
    }
    return theme.background;
}

void Theme::set(Theme theme) {
    g_theme = theme;
    // Restyle immediately: without this the new palette only reaches pixels after
    // some unrelated invalidation, so a switch looked half-applied.
    Application::invalidate_all_windows();
}

Theme Theme::make_dark() {
    Theme t;
    t.dark = true;
    t.background = Color{0x1E, 0x1E, 0x1E};
    t.text = Color{0xE6, 0xE1, 0xE5};
    t.text_secondary = Color{0xCA, 0xC4, 0xD0};
    t.text_disabled = Color{0x93, 0x8F, 0x99};

    t.accent = Color{0x67, 0x50, 0xA4};
    t.accent_hover = Color{0x7D, 0x6B, 0xB8};
    t.accent_pressed = Color{0x53, 0x43, 0x7F};
    t.accent_disabled = Color{0x4A, 0x44, 0x58};
    t.accent_text = Color{0xFF, 0xFF, 0xFF};

    t.surface = Color{0x2B, 0x29, 0x30};
    t.surface_container_low = Color{0x25, 0x23, 0x29};
    t.surface_container = Color{0x2B, 0x29, 0x30};
    t.surface_container_high = Color{0x35, 0x32, 0x3B};
    t.border = Color{0x7B, 0x74, 0x8B};
    t.border_hover = Color{0x93, 0x8C, 0x9F};
    t.track = Color{0x49, 0x45, 0x4F};

    t.selection_bg = Color{0x67, 0x50, 0xA4, 0xA6};
    t.selection_text = Color{0xE6, 0xE1, 0xE5};
    return t;
}

Theme Theme::make_light() {
    Theme t;
    t.dark = false;
    t.background = Color{0xFA, 0xF8, 0xFF};
    t.text = Color{0x1C, 0x1B, 0x1F};
    t.text_secondary = Color{0x49, 0x45, 0x4F};
    t.text_disabled = Color{0x93, 0x8F, 0x99};

    t.accent = Color{0x67, 0x50, 0xA4};
    t.accent_hover = Color{0x7A, 0x65, 0xB5};
    t.accent_pressed = Color{0x53, 0x43, 0x7F};
    t.accent_disabled = Color{0xC8, 0xC2, 0xCF};
    t.accent_text = Color{0xFF, 0xFF, 0xFF};

    t.surface = Color{0xF5, 0xF2, 0xFA};
    t.surface_container_low = Color{0xFF, 0xFF, 0xFF};
    t.surface_container = Color{0xF5, 0xF2, 0xFA};
    t.surface_container_high = Color{0xEC, 0xE7, 0xF2};
    t.border = Color{0x7A, 0x75, 0x7F};
    t.border_hover = Color{0x8A, 0x84, 0x90};
    t.track = Color{0xE6, 0xE0, 0xE9};

    t.selection_bg = Color{0x67, 0x50, 0xA4, 0xA6};
    t.selection_text = Color{0x1C, 0x1B, 0x1F};
    return t;
}

}  // namespace yzk