# YuzukiUI — Developer Guide & Development Status

> For the product pitch and vision, see [VISION.md](VISION.md). This document is for
> developers: how the framework works, what is implemented, and where the project is going.

## Overview

YuzukiUI is a low-footprint, retained-mode C++17 UI framework for Windows. It renders
with Direct2D / DirectWrite / DXGI and has no third-party dependencies — only Windows
system APIs. Windows are trees of widgets; changes to the tree or widget state trigger
on-demand repainting of only the damaged regions.

- **Version:** 0.2.0 (alpha)
- **Platform:** Windows 10+ (x64), Visual Studio 2022 / MSVC
- **Language:** C++17
- **License:** MIT
- **Dependencies:** `d2d1`, `dwrite`, `dxgi`, `d3d11`, `d3dcompiler`, `winmm` (all system libs)

## Frame pipeline (actual)

The per-frame order differs from a classic dirty-driven retained layout system in
specific, intentional ways. This is the authoritative description:

```
Application::run
  Drain        PeekMessage ≤32/frame → wndproc → handlers (mark paint damage)
  Idle         nothing pending → WaitMessage; pending-but-guarded → MsgWait(1ms)
  ── per window with pending paint ──────────────────────────────────────
  tick         tween advance (real time) / frame callbacks / resize broadcast
  Measure      FULL TREE, every painted frame (no layout-dirty tracking;
               g_layout_pass only dedupes within a frame)
  Layout       FULL TREE, every painted frame (set_bounds equality early-out
               prevents damage storms)
  Paint        record (tree → command list; subtree culling on partial frames)
               replay   (command-level damage culling)
  Present      full-surface composite → Present(1,0) — vsync block is the frame pacer
  reconcile    painted-extent deltas → damage for the NEXT frame (converges)
  ── wrapped in a ≤3-attempt retry loop on frame failure (device loss) ──
```

Deliberate deviations from the textbook model, and why:

- **Layout is not dirty-driven.** Every painted frame re-measures and re-arranges the
  whole tree. Correctness is trivially guaranteed and the cost is absorbed by the text
  layout cache (LRU 2048) plus `set_bounds` equality early-outs. Revisit only when
  `FrameStats::layout_ms` averages > 1.5 ms on trees > 2000 visible widgets.
- **The display list is not retained.** Commands are re-recorded from the tree each
  painted frame; partial frames skip work at record time (subtree culling) and replay
  time (command culling) instead of reusing a stored list.
- **Paint damage is an accumulator, not a pipeline stage.** Three producers — event
  handlers during Drain (same-frame), `set_bounds` during Layout (next frame),
  post-Paint reconcile (next frame) — one consumer (replay). It gates Paint only;
  Measure/Layout never consult it.
- **Out-of-band subsystems** the linear model omits: deferred shadow/blur bitmap
  generation runs in its own BeginDraw session outside the frame's; backdrop blur does
  a synchronous GPU readback (`Flush` + copy) inside Paint; multiple windows each run
  the full pipeline sharing one AnimationSystem heartbeat; `WM_PAINT` folds into the
  damage system as a full invalidate.

## Architecture

```
┌────────────────────────────────────────────┐
│  Application (message loop)                │
│  ┌────────────────┐  ┌──────────────────┐  │
│  │ Window          │  │ Widget tree      │  │
│  │  events / hit   │  │ layout / paint   │  │
│  │  damage tracking│  │ (retained mode)  │  │
│  └───────┬────────┘  └────────┬─────────┘  │
│          ▼                    ▼            │
│  ┌──────────────────────────────────────┐  │
│  │ PaintContext → command list          │  │
│  └──────────────┬───────────────────────┘  │
│                 ▼                          │
│  ┌──────────────────────────────────────┐  │
│  │ RenderBackend (abstract interface)   │  │
│  └──────────────┬───────────────────────┘  │
│                 ▼                          │
│  ┌──────────────────────────────────────┐  │
│  │ Direct2D backend (D2D + DWrite + DXGI)│ │
│  └──────────────────────────────────────┘  │
└────────────────────────────────────────────┘
```

### Core pieces

- **Application** — owns the message loop. Drains up to 32 messages per iteration,
  renders each window at most once per iteration (Present blocks on vsync for a stable
  cadence), then calls `WaitMessage()` when idle → a static window costs ~0% CPU.
  Raises the system timer resolution to 1 ms for precise animation timers.
- **Window** — one HWND plus the root widget. Dispatches input, tracks damage regions,
  drives animations, and coordinates layout + paint each frame.
- **Widget** — base class of everything. Owns children, layout data (bounds, margins,
  min/max size, flex), visual state (translate / rotate / scale / opacity, with implicit
  transitions), painting, and event handling. Floating widgets (context menus, popups)
  override `participates_in_layout()` to anchor themselves; parent layouts recurse into
  them for their own layout pass but keep them out of flow math — otherwise two writers
  on bounds fight every frame and cause perpetual invalidation.
- **PaintContext** — records draw calls into a command list each frame (window-space
  coords with bounding boxes), then replays only commands intersecting the dirty rects.
- **RenderBackend** — abstract render interface (currently a single Direct2D
  implementation; Windows-only by design, see VISION.md).

## Rendering pipeline

- **Command list + dirty-rect replay** — the widget tree is traversed once per frame to
  record commands; each dirty rect is replayed with GPU clipping so only damaged areas rasterize.
- **Widget-level culling** — subtrees whose last painted bounds intersect no dirty rect are
  skipped during record.
- **Damage merging** — intersecting dirty rects merge with a bounded area (cap 32 rects, then
  full-frame fallback) to avoid chain-expansion.
- **Deferred shadows** — shadow bitmaps are generated off the critical path; the frame after
  generation is re-rendered once so shadows appear without a hitch. Sizes are quantized to a
  4 DIP grid so animations reuse cached shadow maps.
- **Backdrop blur** — snapshots read accumulated frame content; a dirty rect touching a
  blur region forces a full repaint of that region to avoid stale-pixel ghosting.
- **Text** — fonts are cached per frame (family/size/weight/italic spec), glyphs rasterized
  by DirectWrite; text layout and caret/selection geometry are backend-provided.
- **DPI** — per-monitor DPI aware (v2). All layout works in DIPs; pixels scale by `dpi/96`.

## Event system

`Event` struct with a typed payload (`MouseData` / `KeyData`) and a `consumed` flag.

- **Mouse:** move / enter / leave, down / up / dblclk (left/right/middle), wheel, click
- **Keyboard:** key down / up, character, modifier state, repeat detection
- **Window:** resize (coalesced — one broadcast per rendered frame, whole tree,
  parents first, delivered before that frame's layout), focus gained / lost
- **App:** timer (per-widget, one per widget), drag start / move / end

Hit testing walks the widget tree top-down; events bubble from the target up through
parents. Hover is tracked and repainted incrementally. `TrackMouseEvent` arms leave
notifications. Widgets can be marked `focusable`, get a visible focus ring when navigating
with Tab, and controls expose virtual callbacks (`on_click`, `on_toggled`, `on_selected`,
`on_changed`, ...) instead of signal/slot machinery.

**Timing contract:** all `on_event` handlers execute synchronously inside the message
drain loop (no yield point). Handlers MUST return in < 1 ms; long-running work (I/O,
network, decode) must be deferred to the frame boundary. Violating this stalls every
window and every animation.

## Layout system

All containers inherit `Layout` and arrange children in **local coordinates** (child
bounds are relative to the parent; the paint/hit paths compose offsets along the chain).

- **StackPanel** — horizontal / vertical stacking with spacing and cross alignment
- **FlexBox** — main-axis distribution, grow / shrink flex factors, cross alignment
- **GridPanel** — row/column definitions with fixed, auto and star (weighted) sizing
- **DockPanel** — dock children to edges, fill remainder
- **WrapPanel** — flow children into rows or columns

Every widget carries margins, min/max size, flex grow/shrink, and `measure`/`perform_layout`
virtuals. Custom containers override `arrange_content(area)`.

## Animation system

- **Tween** — one value `from → to` over time with an easing function (10 easings:
  linear, quad, cubic, back — in/out/inout).
- **AnimationSystem** — global singleton owning all tweens and per-frame callbacks.
  Tweens are referenced by token, not pointer — stale tokens are safely ignored.
- **AnimatableProperty\<T\>** — a value that can be tweened (works for numbers, `Color`,
  `Point`, `RectF`; extend via `PropLerp` specialization). Two idioms:
  - `prop.animate(target, ms, easing)` — explicit tween
  - `prop.set_transition(ms); prop = newValue;` — implicit CSS-like transition
- **Frame callbacks** — `on_frame(cb)` drives per-render-frame animation (tweens sample
  real time inside `pump`), avoiding WM_TIMER starvation under input floods.
- Animation timers are only alive while tweens/frame callbacks exist; idle windows stop them.

## Theming

A single `Theme` struct: background/text colors, accent palette, surfaces, borders, fonts,
corner radius, spacing, padding, and a `dark` flag. `Theme::make_dark()/make_light()` and
`Theme::set()`. Widgets read `ctx.theme()` during paint; controls pick their own colors
from the theme, and `Theme::set` invalidates every open window so a switch restyles the
whole app immediately.

## Icon system

Vector icons rendered as text glyphs from the Phosphor font (MIT) — crisp at any DPI,
no bitmap assets. The library exposes `IconId` (~100 common icons bundled alongside the
font) and `icon_glyph(id)`. Load `Phosphor.ttf` via `backend().add_font_file(...)` and
draw with the `icon_family` font; `icofont_demo` renders the full set for eyeballing
codepoints.

## Window features

- Standard titled windows and **borderless** windows (`set_borderless(true)`) with
  custom caption widgets (`set_caption`), edge resizing, and taskbar-aware maximize.
- Per-monitor DPI with `WM_DPICHANGED` handling.
- Minimal move/resize flicker: dirty-rect rendering keeps redraws small during sizing.
- **IME composition**: text-entry widgets declare `wants_ime()`; the system pre-edit
  window is suppressed (`WM_IME_SETCONTEXT`) and TextBox renders the composing string
  inline at the caret with an underline, while the candidate list is anchored to the
  caret via `ImmSetCompositionWindow/CandidateWindow`. Password fields disable the IME
  entirely (`ImmAssociateContext`). Events: `ImeCompose` / `ImeCommit`.

## Controls inventory

| Category  | Controls |
| --------- | -------- |
| Basic     | Label, Button, Icon, Image, Box |
| Input     | TextBox (single / password / multiline), SpinBox, Slider, ComboBox |
| Selection | CheckBox, RadioButton, ToggleSwitch |
| Lists     | ListView (virtualized via DataSource, custom row delegates) |
| Feedback  | ProgressBar (determinate / indeterminate), Notification, Tooltip |
| Menus     | ContextMenu |
| Panels    | StackPanel, FlexBox, GridPanel, DockPanel, WrapPanel |
| Effects   | BackdropBlur, Overlay (modal / drop-down, animated) |

`ListView` virtualizes large data sets: rows are produced on demand by a `DataSource` and
only visible rows are laid out, hit-tested, and painted — 10,000+ rows render like a handful.

## Examples

| Example        | What it demonstrates                                           |
| -------------- | -------------------------------------------------------------- |
| hello          | Minimal app                                                    |
| controls_demo  | Every built-in control and its events                          |
| animation_demo | Tweens, transitions, effects                                   |
| transform_demo | Visual transforms and auto-animation                           |
| icofont_demo   | 100 Phosphor icons in a virtualized grid; click-to-copy codepoint |
| window_demo    | Borderless windows, custom captions, resize, maximize          |
| render_demo    | Gradients, shadows, blur, clipping, paint order                |
| layout_test    | Layout invariants for every panel type (runnable assertion suite) |
| perf_demo      | 2000-widget grid, dirty-rect partial repaint, frame stats HUD  |
| codex_ui       | A complete chat UI built on the framework                      |

## Building & testing

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release
```

Run any demo from `build\examples\Release\<name>.exe`. Unit tests cover encoding, geometry,
layout invariants, animation math, and core utilities (169 checks).

## Development progress

### Implemented

- Core: message loop, window management, widget tree, event dispatch, hit testing, focus
- Rendering: command-list recording, dirty-rect partial frames, widget culling, gradients,
  shadows (deferred + cached), backdrop blur, rounded clipping, text, bitmaps (WIC), DPI
- Layout: stack / flex / grid / dock / wrap panels with margins, min/max, flex factors
- Animation: tweens, 10 easings, implicit transitions, animatable properties, frame callbacks
- Theming: dark / light
- Icon system: Phosphor font, 100 verified `IconId`s, vector rendering
- Window: borderless + custom caption + edge resize + taskbar-aware maximize
- Controls: 24 controls across 8 categories, virtualized ListView
- Diagnostics: `FrameStats::full_damage_clamp` (32-rect cap降级), debug-only per-widget
  `measure_ms()`, `PaintCommand::source` widget pointer for profiling / a11y,
  `Widget::reset_painted_bounds()` for expand/collapse culling recovery
- Examples: 10 runnable demos including a 2000-widget perf test
- Tests: 174 checks passing
- IME: inline composition in TextBox, caret-anchored candidate window, password opt-out

### Next

Roadmap for 0.3.0: harden the core, cut frame cost, and close the visual quality gaps in
effects. No cross-platform backends are planned — Yuzuki stays Windows + Direct2D
(see [VISION.md](VISION.md)).

#### Breaking changes (0.3.0)

APIs removed after the dead-code audit (no in-repo callers; compile breaks are
intentional — flag them in downstream ports):

- `RenderBackend::info()` / `BackendInfo`
- `RenderBackend::begin_frame(clear, clip_dip)` → `begin_frame(clear)` (the clip
  parameter was always nullptr; damage clipping lives in `begin_damage_rect`)
- `Window::set_title`
- `Widget::set_tag / tag`
- `PaintContext::draw_text_title` (+ the title font cache entry)
- `Tween::set_duration / set_easing / set_on_update`
- `Application::register_class / pump_window` (declared, never defined)

Behavior changes:

- `EventType::Resize` is now actually delivered (it previously existed only as an
  enum value): coalesced to one broadcast per rendered frame, whole tree, parents
  first, before that frame's layout; same-size `WM_SIZE` messages are dropped.
- `Theme::set` invalidates every open window (switches restyle immediately).
- Fonts: default family is Lexend Deca (`uiFonts/regular`); `add_font_file`
  accumulates instead of replacing the set.

#### Effects (visual quality)

- [x] Sweep gradient seam: fixed — the shader rotates samples into the sweep frame so
      `atan2`'s branch cut lands exactly at the arc start, full sweeps fade back to the
      first stop over the final 20% with smoothstep (no band at 0°/360°), partial arcs
      clamp instead of wrapping. Multi-stop support added: `fill_sweep_gradient_stops`
      interpolates up to 8 stops piecewise-linearly (extras evenly reduced, unsorted
      input normalized); the shader now compiles at runtime from embedded source via
      d3dcompiler, replacing the hand-maintained CSO blob (`src/render/d2d/sweep_effect.*`).
- [x] Backdrop blur banding: the whole compositing chain (layer, blur snapshot) now
      renders in 16-bit float — matching the shadow path — so soft gradients never
      quantize into visible bands; the only 8-bit step is the final Present.
- [x] Shadow cache eviction: the 64-entry cache used to clear entirely on overflow,
      thrashing under many distinct shadow specs. Entries now carry a monotonic
      last-use stamp and eviction drops only the least recently used shadow
      (`src/render/d2d/d2d_effects.cpp`).
- [x] Shadow size quantization: the fixed 4 DIP grid (`kShadowGrid`) made shadows step
      visibly when controls animated size. The grid now adapts to blur — soft shadows
      keep the coarse reuse-friendly grid, crisp ones refine down to 1 DIP
      (grid = clamp(blur * 0.5, 1, kShadowGrid), `src/render/d2d/d2d_effects.cpp`).

#### Performance

- [x] Gradient brush reuse: `fill_gradient` / `fill_radial_gradient` recreated gradient
      stop collections and brushes on every draw. Brushes are now cached LRU-bounded
      (128 entries) by (kind, extent, color pair) and positioned per draw via a brush
      translation, so one brush serves every rect of the same size; the cache is
      cleared on device loss (`src/render/d2d/d2d_shapes.cpp`). Measured ~54x faster
      on repeated gradient fills.
- [x] Sweep gradient cost: the input snapshot is now cached by (size, dpi) and the
      rounded mask geometry reuses the shared rounded-geometry cache; the per-draw
      `Flush()` sync was dropped (same-context ordering suffices)
      (`src/render/d2d/d2d_shapes.cpp`).
- [x] Backdrop blur churn: the gaussian-blur effect is created once per device and
      updated via SetInput/SetValue, and the snapshot bitmap is FP16 matching the
      layer. The `Flush()` before the snapshot read-back copy stays — layer_ is the
      live render target there and skipping the sync made the panel capture last
      frame's pixels (flicker on page changes) (`src/render/d2d/d2d_effects.cpp`).
- [x] Offscreen compositing: measured before attempting — the layer->swapchain
      composite pass costs ~0.2 ms per frame at 800x600 (steady state; ~1 ms linear
      estimate at 4K) against a 16 ms vsync budget. A direct-to-swapchain fast path
      would only apply to full-repaint frames (FLIP_DISCARD gives the swapchain no
      persistence, partial frames depend on the persistent layer), so the complexity
      was declined. `RenderBackend::composite_ms_avg()` keeps this measurable.
- [x] Partial present: the Present1 dirty-rect path was unreachable under FLIP_DISCARD
      and logically inapplicable — the layer framebuffer is copied to the swapchain in
      full every frame, so there is nothing to partially present. The dead path was
      removed; dirty-rect policy lives entirely in the layer replay
      (`src/render/d2d/d2d_backend.cpp`).

#### Core robustness

- [x] Resource invalidation: after device loss `BitmapId` handles used to dangle
      (`bitmaps_` was cleared) and context-bound caches (clip layers, visual layers,
      cached geometry, sweep effect) leaked into the new device, failing the next
      `EndDraw` with `D2DERR_WRONG_RESOURCE_DOMAIN`. Bitmap WIC sources are now kept so
      device bitmaps rebuild lazily and ids stay valid; all device-scoped caches are
      reset in `destroy_target` (`src/render/d2d/d2d_bitmap.cpp`, `d2d_backend.cpp`).
- [x] Bitmap lifecycle: added `RenderBackend::unload_bitmap(BitmapId)` which releases
      the device bitmap + decoded source; trailing dead slots compact so ids of live
      bitmaps stay stable (`src/render/d2d/d2d_bitmap.cpp`).
- [x] Font registration: `add_font_file` used to replace the whole font set on every
      call, so loading a second font (e.g. the icon font) dropped the first — fixed by
      accumulating all registered `IDWriteFontFile`s and rebuilding the collection per
      load (`src/render/d2d/d2d_text.cpp`).
- [x] Text hit-testing: caret/selection queries used to lay out at a fake 1e7 height.
      Hit layouts now wrap at the real content height, TextBox hit-testing uses the same
      wrap width as painting, and vertical caret movement derives line height from the
      loaded font (fixed-height assumptions drifted with CJK fallback fonts) and clamps
      into the content (`src/render/d2d/d2d_text.cpp`, `src/controls/text_box.cpp`).

## Design principles

1. **Simple** — everything is a widget; nesting is composition; learn one control, learn them all.
2. **Free** — prebuilt controls are examples, not the core; `paint_impl` is always the escape hatch;
   animation is just a property that moves.
3. **Fast** — fine-grained dirty rects + command lists + deferred shadows + GPU rendering; measured,
   not assumed.

See [VISION.md](VISION.md) for the product vision behind these principles.