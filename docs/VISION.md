# YuzukiUI — Vision

> **One-liner:** Write UI like a script, run as fast as native, compose as freely as LEGO.
>
> **Positioning:** The strongest GUI framework on Windows.

## Three pillars

### 1. Simple — a middle-schooler can write it after one glance at the API

- Everything is a Widget, no exceptions; nesting is composition
- Look is a property, not code: `fill / radius / border / shadow` configured declaratively
- The API is table-driven: learn one control, learn them all

### 2. Free — any component, any look, any animation

- Prebuilt controls are examples, not the API core
- The `paint_impl` escape hatch always stays
- Animation = properties that move; tween any property in one line

### 3. Fast — an order of magnitude faster than the same category

- Fine-grained dirty rects + command lists + deferred shadows + GPU rendering
- Keep squeezing the rendering pipeline

## What we explicitly don't do

- No cross-platform: Windows only — the only way to reach extreme simplicity and extreme speed
- No kitchen-sink control library: controls are swappable examples, bring your own
- No commercial-license baggage: open source, take it and use it

## Decision checklist — three questions

1. Can a middle-schooler guess this API without reading the docs?
2. Can a user achieve this look without writing any paint code?
3. Does this change make rendering slower?

## Benchmark

Qt's completeness + ImGui's simplicity + Flutter's freedom — Windows only.