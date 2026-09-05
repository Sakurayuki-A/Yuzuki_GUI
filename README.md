# YuzukiGUI

A low-footprint, retained-mode C++ UI framework for Windows — built with the hope
that it becomes the most delightful GUI framework out there. The concrete target,
the best and most realistic one: Electron's development experience — write the UI
like a script, watch it rebuild and rerun in seconds, compose pages out of plain
widgets — on a native GUI's resource footprint: tens of MB of memory, near-idle CPU
when nothing animates. Not a Chromium in a trench coat.

Today, Yuzuki is already zero-dependency and hardware accelerated, with one header to
include and a few lines to your first window. A static window sits at near-0% CPU idle
(no render loop — measured ~0% on a modern machine; only continuous animations, like an
indeterminate progress bar, drive a small constant load). It's not Electron-smooth yet,
but that's where it's going.

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

## The vision: Electron's DX, native's footprint

What "Electron's development experience on a native resource budget" means for
Yuzuki's future — the best and most realistic target:

- **Retained mode, zero magic**: a window is a tree of widgets. Stack them, dock them, flex them — what you see is what you wrote.
- **Script-like iteration**: one header, one file, rebuild and rerun in seconds. No bundlers, no layers of tooling between you and pixels.
- **One concept at a time**: `Label`, `Button`, `ListView`, `Slider`... each control does exactly what its name says.
- **Events by overriding**: want a button to do something? Subclass it and override `on_click()`. No callbacks, no signal spaghetti.
- **Everything is optional**: dark theme, animations, icons, borderless windows — add them when you need them.
- **Native, not Chromium**: that development loop must ride on tens of MB and near-idle CPU, not a bundled browser.

## What's inside

Buttons, text boxes, lists, sliders, combo boxes, tabbed pages, menus, message boxes,
native file dialogs, keyboard accelerators, clipboard, notifications, tooltips,
rich text labels, undo/redo text editing, modal secondary windows,
flex / grid / dock / wrap layouts, dark & light themes, tweens and transitions,
vector icon fonts, images, virtualized scrolling, and a perf demo with a 2000-widget
grid that repaints only what changed.

## Examples

| Example        | What it shows                                                    |
| -------------- | ---------------------------------------------------------------- |
| hello          | Minimal app — start here (docs/TUTORIAL.md)                      |
| playground     | Iterative playground over every widget, layout, and event        |
| animation_demo | Animations, transitions, effects                                 |
| icofont_demo   | 100 Phosphor icons in a virtualized grid; click to copy codepoints |
| window_demo    | Borderless windows, custom captions, resize, maximize            |
| render_demo    | Gradients, shadows, blur, clipping                               |
| layout_test    | Layout invariants across every panel type                        |
| perf_demo      | 2000-widget tree with dirty-rect partial repaint                 |
| explorer       | A file explorer built on the framework                            |
| codex_ui       | A full chat UI built on the framework                            |
| debug_demo     | DebugOverlay (F1) and WidgetInspector (F2) walkthrough           |
| api_validate   | Runnable transcription of docs/API.md — docs verified as code    |
| app_shell      | Clone-and-start template: app icon + DPI/ComCtl manifest         |
| cookbook_*     | Cookbook pages — settings, login, chat, dialogs (docs/COOKBOOK.md) |

## Open-source fonts

YuzukiUI is built on the shoulders of two great open typefaces:

- **[Phosphor](https://phosphoricons.com)** — the icon font behind the vector icon
  system (`IconId` / `icon_glyph`). MIT licensed.
- **[Lexend Deca](https://fonts.google.com/specimen/Lexend+Deca)** — from the
  [Lexend](https://lexend.com) family, the default UI font shipped with the demos.
  SIL Open Font License.

## Status

0.3.0 alpha. Windows 10+, Visual Studio 2022, Direct2D backend.

[MIT](LICENSE) — free to use commercially.
