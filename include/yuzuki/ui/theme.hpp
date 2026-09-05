#pragma once
#include <yuzuki/core/types.hpp>

namespace yzk {

// Semantic theme role for widgets whose color should follow the active theme at
// paint time (instead of a snapshot Color). Resolved from Theme::get() every
// repaint so a theme swap re-skins widgets automatically.
enum class ThemeRole : u8 {
    Background,
    Surface,
    SurfaceContainerLow,
    SurfaceContainer,
    SurfaceContainerHigh,
    Border,
    Track,
    Accent,
    AccentText,
    Text,
    TextSecondary,
    TextDisabled,
};

struct Theme {
    Color background{0xF3, 0xF3, 0xF3};
    Color text{0x1E, 0x1E, 0x1E};
    Color text_secondary{0x6E, 0x6E, 0x6E};
    Color text_disabled{0xA8, 0xA8, 0xA8};

    Color accent{0x67, 0x50, 0xA4};
    Color accent_hover{0x7D, 0x6B, 0xB6};
    Color accent_pressed{0x52, 0x3E, 0x83};
    Color accent_disabled{0xC1, 0xB5, 0xE3};
    Color accent_text{0xFF, 0xFF, 0xFF};

    Color surface{0xFA, 0xFA, 0xFA};
    Color surface_container_low{0xF7, 0xF4, 0xF9};
    Color surface_container{0xF3, 0xED, 0xF7};
    Color surface_container_high{0xEC, 0xE6, 0xEE};
    Color border{0xD0, 0xD0, 0xD0};
    Color border_hover{0xAD, 0xAD, 0xAD};

    Color track{0xE6, 0xE0, 0xE9};  // progress bar / slider track (Material: surface-variant)

    Color selection_bg{0x5A, 0x8F, 0xFF, 0xA6};
    Color selection_text{0x1E, 0x1E, 0x1E};

    // ---- Typography scale (Phase 2.2): semantic roles. ----
    // type_body == font_size, type_caption == font_size_small; the *_size fields are
    // the raw values paint() reads, type_* are the semantic names apps should use.
    f32 type_display = 34.0f;
    f32 type_headline = 28.0f;
    f32 type_title = 20.0f;
    f32 type_body = 14.0f;
    f32 type_label = 13.0f;
    f32 type_caption = 12.0f;

    String font_family = "Lexend Deca";
    f32 font_size = 14.0f;
    f32 font_size_small = 12.0f;
    f32 font_size_title = 18.0f;

    // ---- Geometry tokens (Phase 2.1) ----
    // Standard interactive control height; compact controls (check, radio, toggle)
    // use a smaller height via their own const until migrated.
    f32 control_height = 32.0f;
    f32 control_height_compact = 28.0f;

    // Fixed control radius for inputs/buttons; radius_sm/md raised for cards.
    f32 control_radius = 4.0f;
    f32 radius_sm = 4.0f;
    f32 radius_md = 8.0f;
    f32 radius_pill = 999.0f;

    f32 border_width = 1.0f;
    f32 border_width_strong = 1.5f;

    f32 scrollbar_width = 6.0f;
    f32 scrollbar_margin = 2.0f;
    f32 wheel_step = 40.0f;
    f32 scrollbar_thumb_min = 24.0f;

    // ---- Elevation shadows (Phase 2.4): semantic levels, not per-control numbers. ----
    // Floating is for menus / dropdowns / tooltips (navigation surfaces);
    // notice is for transient feedback (notifications, toasts) — lighter by intent.
    // blur only; draw_shadow() applies these together.
    f32 shadow_blur_floating = 14.0f;
    f32 shadow_offset_floating = 4.0f;
    u8 shadow_alpha_floating = 70;
    f32 shadow_blur_notice = 12.0f;
    f32 shadow_offset_notice = 4.0f;
    u8 shadow_alpha_notice = 40;

    f32 corner_radius = 4.0f;
    f32 spacing = 8.0f;
    f32 padding = 8.0f;

    bool dark = true;  // dark-theme flag (controls pick shadow/glow colors by theme)

    static const Theme& get();
    static void set(Theme theme);
    static Theme make_dark();
    static Theme make_light();
};

// Resolve a ThemeRole against a theme instance; returns the current palette entry.
const Color& theme_color(const Theme& theme, ThemeRole role);

}  // namespace yzk