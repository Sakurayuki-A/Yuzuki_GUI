# YuzukiGUI

A low-footprint, retained-mode C++ UI framework for Windows — built with the hope
that it becomes the most delightful GUI framework out there. And that's not where
the ambition stops: the future of Yuzuki should be like LEGO — so simple that a
middle schooler can pick it up in an afternoon and start building.

Today, Yuzuki is already zero-dependency and hardware accelerated, with one header to
include and a few lines to your first window. A static window sits at near-0% CPU idle
(no render loop — measured ~0% on a modern machine; only continuous animations, like an
indeterminate progress bar, drive a small constant load). It's not LEGO yet, but that's
where it's going.

```cpp
#include <yuzuki/yuzuki.hpp>
using namespace yzk;

int main() {
    Application& app = Application::instance();

    Window window("My App", 480, 320);
    if (!window.create()) return 1;

    auto root = new StackPanel(Orientation::Vertical);
    root->append_child(new Label("Hello YuzukiUI"));
    root->append_child(new Button("Click Me"));

    window.set_root(root);
    window.show();
    return app.run();
}
```

That's it. No makefiles to fight, no build systems to learn — just build and run:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
build\examples\Release\hello.exe
```

## The LEGO vision

What "simple like LEGO" means for Yuzuki's future:

- **Retained mode, zero magic**: a window is a tree of widgets. Stack them, dock them, flex them — what you see is what you wrote.
- **One concept at a time**: `Label`, `Button`, `ListView`, `Slider`... each control does exactly what its name says.
- **Events by overriding**: want a button to do something? Subclass it and override `on_click()`. No callbacks, no signal spaghetti.
- **Everything is optional**: dark theme, animations, icons, borderless windows — add them when you need them.

## What's inside

Buttons, text boxes, lists, sliders, combo boxes, menus, notifications, tooltips,
flex / grid / dock / wrap layouts, dark & light themes, tweens and transitions,
vector icon fonts, images, virtualized scrolling, and a perf demo with a 2000-widget
grid that repaints only what changed.

## Examples

| Example        | What it shows                                                    |
| -------------- | ---------------------------------------------------------------- |
| hello          | Minimal app                                                      |
| controls_demo  | Every built-in control and its events                            |
| animation_demo | Animations, transitions, effects                                 |
| transform_demo | Visual transforms and auto-animation                             |
| icofont_demo   | 100 Phosphor icons in a virtualized grid; click to copy codepoints |
| window_demo    | Borderless windows, custom captions, resize, maximize            |
| render_demo    | Gradients, shadows, blur, clipping                               |
| layout_test    | Layout invariants across every panel type                        |
| perf_demo      | 2000-widget tree with dirty-rect partial repaint                 |
| codex_ui       | A full chat UI built on the framework                            |

## Status

0.2.0 alpha. Windows 10+, Visual Studio 2022, Direct2D backend.

[MIT](LICENSE) — free to use commercially.
