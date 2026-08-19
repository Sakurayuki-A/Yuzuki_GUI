#pragma once
#include <yuzuki/core/types.hpp>

namespace yzk {

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

    String font_family = "Satoshi";
    f32 font_size = 14.0f;
    f32 font_size_small = 12.0f;
    f32 font_size_title = 18.0f;

    f32 corner_radius = 4.0f;
    f32 spacing = 8.0f;
    f32 padding = 8.0f;

    bool dark = true;  // dark-theme flag (controls pick shadow/glow colors by theme)

    static const Theme& get();
    static void set(Theme theme);
    static Theme make_dark();
    static Theme make_light();
};

}  // namespace yzk