// Visual regression tests: off-screen render widget trees, capture pixels, compare hashes.
#include <yuzuki/yuzuki.hpp>

#include <windows.h>
#include <objbase.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

// ===== Minimal test harness (same pattern as test_core.cpp) =====
static int g_failures = 0;
static int g_checks = 0;

static void check(bool expr, const char* file, int line, const char* expr_str) {
    ++g_checks;
    if (!expr) {
        ++g_failures;
        std::printf("FAIL %s:%d  %s\n", file, line, expr_str);
    }
}
#define CHECK(expr) check((expr), __FILE__, __LINE__, #expr)

// ===== PNG save via WIC =====
static bool save_png(const fs::path& path, const uint8_t* bgra, uint32_t w, uint32_t h) {
    const HRESULT init_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool com_owned = SUCCEEDED(init_hr);
    bool ok = true;
    {
        Microsoft::WRL::ComPtr<IWICImagingFactory> wic;
        Microsoft::WRL::ComPtr<IWICBitmapEncoder> enc;
        Microsoft::WRL::ComPtr<IWICStream> stream;
        Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frame;
        IPropertyBag2* props = nullptr;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_PPV_ARGS(&wic))) ||
            FAILED(wic->CreateEncoder(GUID_ContainerFormatPng, nullptr, &enc)) ||
            FAILED(wic->CreateStream(&stream)) ||
            FAILED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE)) ||
            FAILED(enc->Initialize(stream.Get(), WICBitmapEncoderNoCache))) {
            ok = false;
        } else if (FAILED(enc->CreateNewFrame(&frame, &props)) ||
                   FAILED(frame->Initialize(props))) {
            ok = false;
        } else {
            frame->SetSize(w, h);
            GUID fmt = GUID_WICPixelFormat32bppBGRA;
            frame->SetPixelFormat(&fmt);
            const uint32_t stride = w * 4;
            ok = SUCCEEDED(frame->WritePixels(h, stride, h * stride,
                                              const_cast<uint8_t*>(bgra)));
            ok = SUCCEEDED(frame->Commit()) && ok;
            ok = SUCCEEDED(enc->Commit()) && ok;
        }
        if (props) props->Release();
    }
    if (com_owned) CoUninitialize();
    return ok;
}

// ===== FNV-1a hash for pixel comparison =====
static uint64_t pixel_hash(const uint8_t* data, size_t len) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; ++i) {
        h ^= data[i];
        h *= 1099511628211ULL;
    }
    return h;
}

// ===== Reference hash database =====
static std::string g_baseline_path;

static std::unordered_map<std::string, uint64_t>& ref_hashes() {
    static std::unordered_map<std::string, uint64_t> m;
    return m;
}

static void load_baselines() {
    if (g_baseline_path.empty()) return;
    std::ifstream f(g_baseline_path);
    if (!f.is_open()) return;
    auto& refs = ref_hashes();
    std::string line;
    while (std::getline(f, line)) {
        auto pos = line.find(' ');
        if (pos == std::string::npos) continue;
        std::string name = line.substr(0, pos);
        std::string hex = line.substr(pos + 1);
        uint64_t hash = 0;
        if (sscanf_s(hex.c_str(), "%llx", &hash) == 1) {
            refs[name] = hash;
        }
    }
    std::printf("Loaded %zu baselines from %s\n", refs.size(), g_baseline_path.c_str());
}

static void save_baselines() {
    if (g_baseline_path.empty()) return;
    std::ofstream f(g_baseline_path);
    if (!f.is_open()) {
        std::printf("WARNING: cannot write baselines to %s\n", g_baseline_path.c_str());
        return;
    }
    auto& refs = ref_hashes();
    for (auto& [name, hash] : refs) {
        f << name << " " << std::hex << hash << std::dec << "\n";
    }
    std::printf("Saved %zu baselines to %s\n", refs.size(), g_baseline_path.c_str());
}

static bool g_record_mode = false;

static void check_or_record(const char* name, uint64_t actual_hash) {
    auto& refs = ref_hashes();
    if (g_record_mode) {
        refs[name] = actual_hash;
        std::printf("RECORD %s = 0x%016llx\n", name, (unsigned long long)actual_hash);
        return;
    }
    auto it = refs.find(name);
    if (it == refs.end()) {
        refs[name] = actual_hash;
        std::printf("NEW REF %s = 0x%016llx\n", name, (unsigned long long)actual_hash);
        return;
    }
    CHECK(it->second == actual_hash);
    if (it->second != actual_hash) {
        std::printf("  MISMATCH %s: expected 0x%016llx, got 0x%016llx\n", name,
                     (unsigned long long)it->second, (unsigned long long)actual_hash);
    }
}

// ===== Visual test harness =====
// Creates a hidden Window, renders widget trees, captures pixels via public API.
class VisualHarness {
public:
    VisualHarness(uint32_t w, uint32_t h) : w_(w), h_(h),
        win_(yzk::utf::to_utf8(L"VisualTest"), w, h) {}

    bool init() {
        if (!win_.create()) return false;
        // Register default font
        wchar_t exe[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, exe, MAX_PATH);
        fs::path dir = fs::path(exe).parent_path();
        yzk::String font8 = yzk::utf::to_utf8((dir / L"LexendDeca-Regular.ttf").wstring());
        win_.backend().add_font_file(font8);
        return true;
    }

    yzk::Window& window() { return win_; }

    std::vector<uint8_t> render(yzk::Widget* root) {
        yzk::Theme::set(yzk::Theme::make_dark());
        win_.set_root(root);
        win_.set_capture(true);
        // Force full frame render
        win_.invalidate_all();
        // Bypass 16ms frame guard by pumping twice with a sleep
        Sleep(20);
        win_.pump();
        Sleep(20);
        win_.pump();

        std::vector<uint8_t> pixels;
        uint32_t pw = 0, ph = 0;
        if (!win_.capture_pixels(pixels, pw, ph)) return {};
        win_.release_capture();
        return pixels;
    }

    uint64_t render_hash(yzk::Widget* root) {
        auto px = render(root);
        if (px.empty()) return 0;
        return pixel_hash(px.data(), px.size());
    }

    bool save(const fs::path& path, yzk::Widget* root) {
        auto px = render(root);
        if (px.empty()) return false;
        return save_png(path, px.data(), w_, h_);
    }

    uint32_t width() const { return w_; }
    uint32_t height() const { return h_; }

private:
    uint32_t w_ = 0;
    uint32_t h_ = 0;
    yzk::Window win_;
};

// ===== Helpers =====
// BGRA coarse ASCII dump (debug aid for overlay/text positioning).
static void ascii_dump(const std::vector<uint8_t>& px, uint32_t w, uint32_t h,
                       int cols = 96, int rows = 32) {
    for (int r = 0; r < rows; ++r) {
        std::string line;
        for (int c = 0; c < cols; ++c) {
            const int x = static_cast<int>((c + 0.5f) * w / cols);
            const int y = static_cast<int>((r + 0.5f) * h / rows);
            const uint8_t* p = &px[(static_cast<size_t>(y) * w + x) * 4];  // BGRA
            const float lum = 0.299f * p[2] + 0.587f * p[1] + 0.114f * p[0];
            char ch = ' ';
            if (p[3] >= 128) {
                if (lum > 190) ch = '.';
                else if (lum > 90) ch = ':';
                else if (lum > 40) ch = '#';
                else ch = '@';
            }
            line += ch;
        }
        std::printf("%s\n", line.c_str());
    }
}

// Per-row projection of "text-bright" pixel counts (harder to miss than ASCII).
static void row_projection(const std::vector<uint8_t>& px, uint32_t w, uint32_t h,
                           float x0f, float x1f, float bright, const char* tag) {
    std::printf("-- %s --\n", tag);
    const uint32_t x0 = static_cast<uint32_t>(x0f * w), x1 = static_cast<uint32_t>(x1f * w);
    for (uint32_t y = 0; y < h; ++y) {
        uint32_t n = 0;
        for (uint32_t x = x0; x <= x1; ++x) {
            const uint8_t* p = &px[(static_cast<size_t>(y) * w + x) * 4];
            if (p[3] >= 160 && 0.299f * p[2] + 0.587f * p[1] + 0.114f * p[0] > bright) ++n;
        }
        if (n) std::printf("%3u: %u\n", y, n);
    }
}

// Live-app repro: animated MessageBox on a 640x400 dock root, destroyed while
// on screen (the fire-and-forget pattern). Dumps the final frame as ASCII.
static void debug_animated_overlay_ascii() {
    {
        VisualHarness h(640, 400);
        CHECK(h.init());
        yzk::DockPanel root;
        root.set_min_size(yzk::Size{640.0f, 400.0f});
        h.window().set_root(&root);
        {
            yzk::MessageBox m;
            m.show(h.window(), "Quit", "Quit the editor?", yzk::MessageBoxKind::YesNo);
        }
        for (int i = 0; i < 40; ++i) {
            Sleep(16);
            h.window().pump();
        }
        std::printf("=== animated, dock root, destroyed-on-stack ===\n");
        auto px_a = h.render(&root);
        ascii_dump(px_a, h.width(), h.height());
        row_projection(px_a, h.width(), h.height(), 0.05f, 0.95f, 150.0f, "rows, wide");
    }
    {
        VisualHarness h(400, 200);
        CHECK(h.init());
        yzk::Widget root;
        root.set_min_size(yzk::Size{400.0f, 200.0f});
        h.window().set_root(&root);
        {
            yzk::MessageBox m;
            m.show(h.window(), "Title", "Body text", yzk::MessageBoxKind::Info);
        }
        for (int i = 0; i < 40; ++i) {
            Sleep(16);
            h.window().pump();
        }
        std::printf("=== static, bare root ===\n");
        auto px_b = h.render(&root);
        ascii_dump(px_b, h.width(), h.height());
        row_projection(px_b, h.width(), h.height(), 0.05f, 0.95f, 150.0f, "rows, wide");
    }
}

static yzk::Label* make_label(const yzk::String& text, bool small_font = false) {
    auto* label = new yzk::Label(text);
    if (small_font) label->set_small(true);
    return label;
}

// ===== Test cases =====

static void test_solid_background() {
    VisualHarness h(400, 300);
    CHECK(h.init());
    yzk::Box root;
    root.set_bg(yzk::Theme::get().background);
    auto hash = h.render_hash(&root);
    check_or_record("solid_bg_400x300", hash);
    CHECK(hash != 0);
}

static void test_label_centered() {
    VisualHarness h(400, 300);
    CHECK(h.init());
    yzk::StackPanel panel(yzk::Orientation::Vertical);
    panel.set_padding(16.0f);
    auto* lbl = make_label(yzk::utf::to_utf8(L"Hello Yuzuki"));
    panel.append_child(lbl);
    auto hash = h.render_hash(&panel);
    check_or_record("label_centered", hash);
    CHECK(hash != 0);
    delete lbl;
}

static void test_button_normal() {
    VisualHarness h(200, 80);
    CHECK(h.init());
    yzk::Button btn(yzk::utf::to_utf8(L"Click Me"));
    auto hash = h.render_hash(&btn);
    check_or_record("button_normal", hash);
    CHECK(hash != 0);
}

static void test_stack_panel() {
    VisualHarness h(400, 300);
    CHECK(h.init());
    yzk::StackPanel panel(yzk::Orientation::Vertical);
    panel.set_spacing(8.0f);
    panel.set_padding(16.0f);
    auto* t1 = make_label(yzk::utf::to_utf8(L"Title"));
    auto* t2 = make_label(yzk::utf::to_utf8(L"Subtitle"), true);
    auto* btn = new yzk::Button(yzk::utf::to_utf8(L"Action"));
    panel.append_child(t1);
    panel.append_child(t2);
    panel.append_child(btn);
    auto hash = h.render_hash(&panel);
    check_or_record("stack_panel_3_children", hash);
    CHECK(hash != 0);
    delete t1;
    delete t2;
    delete btn;
}

static void test_box_styled() {
    VisualHarness h(300, 200);
    CHECK(h.init());
    yzk::Box box;
    box.set_bg(yzk::Color{0x2B, 0x29, 0x30});
    box.set_radius(12.0f);
    box.set_border(2.0f, yzk::Color{0x7B, 0x74, 0x8B});
    box.set_padding(16.0f);
    auto* lbl = make_label(yzk::utf::to_utf8(L"Styled Box"));
    box.append_child(lbl);
    auto hash = h.render_hash(&box);
    check_or_record("box_styled_radius_border", hash);
    CHECK(hash != 0);
    delete lbl;
}

static void test_checkbox_unchecked() {
    VisualHarness h(200, 40);
    CHECK(h.init());
    yzk::CheckBox cb(yzk::utf::to_utf8(L"Option A"));
    auto hash = h.render_hash(&cb);
    check_or_record("checkbox_unchecked", hash);
    CHECK(hash != 0);
}

static void test_toggle_off() {
    VisualHarness h(120, 40);
    CHECK(h.init());
    yzk::ToggleSwitch ts;
    auto hash = h.render_hash(&ts);
    check_or_record("toggle_off", hash);
    CHECK(hash != 0);
}

static void test_slider_50() {
    VisualHarness h(300, 40);
    CHECK(h.init());
    yzk::Slider sl;
    sl.set_range(0.0f, 100.0f);
    sl.set_value(50.0f);
    auto hash = h.render_hash(&sl);
    check_or_record("slider_50pct", hash);
    CHECK(hash != 0);
}

static void test_progress_75() {
    VisualHarness h(300, 20);
    CHECK(h.init());
    yzk::ProgressBar pb;
    pb.set_value(0.75f);
    auto hash = h.render_hash(&pb);
    check_or_record("progress_75pct", hash);
    CHECK(hash != 0);
}

static void test_light_theme() {
    VisualHarness h(400, 300);
    CHECK(h.init());
    yzk::Theme::set(yzk::Theme::make_light());
    yzk::StackPanel panel(yzk::Orientation::Vertical);
    panel.set_padding(16.0f);
    auto* lbl = make_label(yzk::utf::to_utf8(L"Light Theme"));
    panel.append_child(lbl);
    auto hash = h.render_hash(&panel);
    check_or_record("label_light_theme", hash);
    CHECK(hash != 0);
    yzk::Theme::set(yzk::Theme::make_dark());
    delete lbl;
}

static void test_grid_2x2() {
    VisualHarness h(400, 400);
    CHECK(h.init());
    yzk::GridPanel grid(2, 2);
    grid.set_column_fixed(0, 200.0f);
    grid.set_column_fixed(1, 200.0f);
    grid.set_row_fixed(0, 200.0f);
    grid.set_row_fixed(1, 200.0f);
    yzk::Box c1, c2, c3, c4;
    c1.set_bg(yzk::Color{0x67, 0x50, 0xA4});
    c2.set_bg(yzk::Color{0x7D, 0x6B, 0xB8});
    c3.set_bg(yzk::Color{0x53, 0x43, 0x7F});
    c4.set_bg(yzk::Color{0x4A, 0x44, 0x58});
    grid.add(&c1, 0, 0);
    grid.add(&c2, 1, 0);
    grid.add(&c3, 0, 1);
    grid.add(&c4, 1, 1);
    auto hash = h.render_hash(&grid);
    check_or_record("grid_2x2_colors", hash);
    CHECK(hash != 0);
}

static void test_spinbox_default() {
    VisualHarness h(150, 40);
    CHECK(h.init());
    yzk::SpinBox sb;
    sb.set_value(42.0);
    auto hash = h.render_hash(&sb);
    check_or_record("spinbox_default", hash);
    CHECK(hash != 0);
}

static void test_combobox() {
    VisualHarness h(200, 40);
    CHECK(h.init());
    yzk::ComboBox cb;
    cb.set_items({yzk::utf::to_utf8(L"Alpha"), yzk::utf::to_utf8(L"Beta"), yzk::utf::to_utf8(L"Gamma")});
    cb.set_selected_index(1);
    auto hash = h.render_hash(&cb);
    check_or_record("combobox_with_items", hash);
    CHECK(hash != 0);
}

static void test_flexbox_grow() {
    VisualHarness h(400, 60);
    CHECK(h.init());
    yzk::FlexBox row;
    row.set_spacing(8.0f);
    yzk::Box fixed, g1, g2;
    fixed.set_bg(yzk::Color{0x67, 0x50, 0xA4});
    g1.set_bg(yzk::Color{0x7D, 0x6B, 0xB8});
    g2.set_bg(yzk::Color{0x53, 0x43, 0x7F});
    g1.set_flex_grow(1.0f);
    g2.set_flex_grow(2.0f);
    row.append_child(&fixed);
    row.append_child(&g1);
    row.append_child(&g2);
    auto hash = h.render_hash(&row);
    check_or_record("flexbox_grow_1_2", hash);
    CHECK(hash != 0);
}

static void test_dock_panel() {
    VisualHarness h(400, 300);
    CHECK(h.init());
    yzk::DockPanel dock;
    yzk::Box top, left, fill;
    top.set_bg(yzk::Color{0x67, 0x50, 0xA4});
    top.set_min_height(40.0f);
    left.set_bg(yzk::Color{0x7D, 0x6B, 0xB8});
    left.set_min_width(80.0f);
    fill.set_bg(yzk::Color{0x2B, 0x29, 0x30});
    dock.append_child(&top);
    dock.append_child(&left);
    dock.append_child(&fill);
    auto hash = h.render_hash(&dock);
    check_or_record("dock_top_left_fill", hash);
    CHECK(hash != 0);
}

static void test_radio_group() {
    VisualHarness h(200, 120);
    CHECK(h.init());
    yzk::StackPanel panel(yzk::Orientation::Vertical);
    panel.set_spacing(4.0f);
    yzk::RadioButton r1(yzk::utf::to_utf8(L"Choice 1"));
    yzk::RadioButton r2(yzk::utf::to_utf8(L"Choice 2"));
    yzk::RadioButton r3(yzk::utf::to_utf8(L"Choice 3"));
    r1.set_checked(true);
    panel.append_child(&r1);
    panel.append_child(&r2);
    panel.append_child(&r3);
    auto hash = h.render_hash(&panel);
    check_or_record("radio_group_3", hash);
    CHECK(hash != 0);
}

static void test_wrap_panel() {
    VisualHarness h(300, 200);
    CHECK(h.init());
    yzk::WrapPanel wrap;
    wrap.set_line_spacing(8.0f);
    for (int i = 0; i < 6; ++i) {
        auto* btn = new yzk::Button("Tag " + std::to_string(i));
        btn->set_min_width(80.0f);
        btn->set_min_height(32.0f);
        wrap.append_child(btn);
    }
    auto hash = h.render_hash(&wrap);
    check_or_record("wrap_panel_6_tags", hash);
    CHECK(hash != 0);
}

static void test_scrollview() {
    VisualHarness h(300, 200);
    CHECK(h.init());
    yzk::ScrollView sv;
    sv.set_suggested_height(500.0f);
    yzk::StackPanel content(yzk::Orientation::Vertical);
    content.set_spacing(4.0f);
    for (int i = 0; i < 20; ++i) {
        auto* lbl = new yzk::Label("Item " + std::to_string(i));
        content.append_child(lbl);
    }
    sv.set_content(&content);
    auto hash = h.render_hash(&sv);
    check_or_record("scrollview_20_items", hash);
    CHECK(hash != 0);
}

static void test_label_alignments() {
    VisualHarness h(400, 200);
    CHECK(h.init());
    yzk::StackPanel panel(yzk::Orientation::Vertical);
    panel.set_spacing(4.0f);
    panel.set_padding(8.0f);
    yzk::Label left(yzk::utf::to_utf8(L"Left aligned"));
    left.set_align(yzk::TextAlignH::Left, yzk::TextAlignV::Center);
    left.set_min_height(30.0f);
    yzk::Label center(yzk::utf::to_utf8(L"Center aligned"));
    center.set_align(yzk::TextAlignH::Center, yzk::TextAlignV::Center);
    center.set_min_height(30.0f);
    yzk::Label right(yzk::utf::to_utf8(L"Right aligned"));
    right.set_align(yzk::TextAlignH::Right, yzk::TextAlignV::Center);
    right.set_min_height(30.0f);
    panel.append_child(&left);
    panel.append_child(&center);
    panel.append_child(&right);
    auto hash = h.render_hash(&panel);
    check_or_record("label_alignments_lcr", hash);
    CHECK(hash != 0);
}

static void test_box_shadow() {
    VisualHarness h(300, 200);
    CHECK(h.init());
    yzk::Box box;
    box.set_bg(yzk::Color{0x2B, 0x29, 0x30});
    box.set_radius(8.0f);
    box.set_shadow(12.0f, 4.0f);
    box.set_padding(16.0f);
    auto* lbl = make_label(yzk::utf::to_utf8(L"Shadow Box"));
    box.append_child(lbl);
    auto hash = h.render_hash(&box);
    check_or_record("box_with_shadow", hash);
    CHECK(hash != 0);
    delete lbl;
}

static void test_device_loss_recovery() {
    VisualHarness h(400, 300);
    CHECK(h.init());

    // Render a frame
    yzk::Label lbl(yzk::utf::to_utf8(L"Before loss"));
    lbl.set_align(yzk::TextAlignH::Center, yzk::TextAlignV::Center);
    lbl.set_min_height(30.0f);
    auto hash1 = h.render_hash(&lbl);
    CHECK(hash1 != 0);

    // Simulate device loss: destroy + recreate target
    auto& backend = h.window().backend();
    bool ok = backend.recreate_after_loss();
    CHECK(ok);

    // Render again - bitmap lazy-rebuild and caches should work
    auto hash2 = h.render_hash(&lbl);
    CHECK(hash2 != 0);

    // Hashes should match on same GPU
    CHECK(hash2 == hash1);
}

static void test_message_box_render() {
    VisualHarness h(400, 300);
    CHECK(h.init());

    yzk::Widget root;
    root.set_min_size(yzk::Size{400.0f, 300.0f});
    h.window().set_root(&root);

    yzk::MessageBox box;
    box.set_animated(false);
    box.show(h.window(), yzk::utf::to_utf8(L"Delete"), yzk::utf::to_utf8(L"Delete the selected rows?"),
             yzk::MessageBoxKind::Confirm);
    CHECK(box.is_open());

    auto hash = h.render_hash(&root);
    check_or_record("message_box_confirm", hash);
    CHECK(hash != 0);
    if (g_record_mode) h.save(fs::path(g_baseline_path).parent_path() / "message_box_confirm.png", &root);

    // Programmatic resolution must close the box and stop rendering it.
    box.resolve(yzk::MessageBoxResult::Ok);
    CHECK(!box.is_open());
    auto hash2 = h.render_hash(&root);
    CHECK(hash2 != 0);
}

static void test_message_box_long_message() {
    VisualHarness h(400, 300);
    CHECK(h.init());

    yzk::Widget root;
    root.set_min_size(yzk::Size{400.0f, 300.0f});
    h.window().set_root(&root);

    yzk::MessageBox box;
    box.set_animated(false);
    const yzk::String long_msg = yzk::utf::to_utf8(
        L"A substantially longer body that should wrap onto multiple lines inside "
        L"the panel instead of overflowing its right edge, so the message box stays "
        L"readable at the default fixed width.");
    box.show(h.window(), yzk::utf::to_utf8(L"Notice"), long_msg, yzk::MessageBoxKind::Info);
    CHECK(box.is_open());

    auto hash = h.render_hash(&root);
    check_or_record("message_box_long_message", hash);
    CHECK(hash != 0);
    if (g_record_mode) h.save(fs::path(g_baseline_path).parent_path() / "message_box_long_message.png", &root);

    box.resolve(yzk::MessageBoxResult::Ok);
    CHECK(!box.is_open());
}

static void test_label_rich_spans() {
    VisualHarness h(400, 120);
    CHECK(h.init());
    yzk::StackPanel panel(yzk::Orientation::Vertical);
    panel.set_padding(16.0f);
    panel.set_spacing(8.0f);

    auto* rich = new yzk::Label(
        yzk::utf::to_utf8(L"This is **bold** and ~highlighted~ plus a [link](open:uuid) here."));
    rich->set_rich_text(true);
    rich->set_align(yzk::TextAlignH::Left, yzk::TextAlignV::Center);
    panel.append_child(rich);

    auto* plain = make_label(yzk::utf::to_utf8(L"Plain row without markup."), true);
    plain->set_align(yzk::TextAlignH::Left, yzk::TextAlignV::Center);
    panel.append_child(plain);

    auto hash = h.render_hash(&panel);
    check_or_record("label_rich_spans", hash);
    CHECK(hash != 0);
    if (g_record_mode) {
        h.save(fs::path(g_baseline_path).parent_path() / "label_rich_spans.png", &panel);
    }
    delete rich;
    delete plain;
}

static void test_d2d_text_hit_test() {
    VisualHarness h(400, 120);
    CHECK(h.init());
    auto& backend = h.window().backend();

    yzk::FontSpec spec;
    spec.family = yzk::Theme::get().font_family;
    spec.size = yzk::Theme::get().font_size;
    const yzk::FontId font = backend.create_font(spec);
    CHECK(font != yzk::kInvalidFont);

    // Single line: caret position -> hit-test must round-trip to about the same
    // character (DirectWrite trailing-half ambiguity allows +/-1).
    const yzk::String line = yzk::utf::to_utf8(L"Hello Yuzuki text hit-test");
    const int n = static_cast<int>(line.size());
    int ok_rt = 0;
    for (int i = 0; i <= n; ++i) {
        const yzk::Point cp = backend.caret_position(font, line, 1e7f, i);
        const int hit = backend.hit_test_text(font, line, 1e7f, cp.x, cp.y);
        if (hit >= i - 1 && hit <= i + 1) ++ok_rt;
    }
    CHECK(ok_rt == n + 1);  // every caret lands back within one cluster

    // Wrapped multi-line: the caret at the text's end sits below the first line,
    // and hit-testing back at that point touches the wrapped segment.
    const yzk::String wrapped = yzk::utf::to_utf8(L"AAAA BBBB CCCC DDDD");
    const float wrap_w = 60.0f;  // narrower than one full line
    const int end = static_cast<int>(wrapped.size());
    const yzk::Point end_cp = backend.caret_position(font, wrapped, wrap_w, end);
    CHECK(end_cp.y > 0.0f);  // wrapped onto a later line
    const int hit_end = backend.hit_test_text(font, wrapped, wrap_w, end_cp.x, end_cp.y);
    CHECK(hit_end >= end - 3 && hit_end <= end);

    // Monotonic caret x within the first line (no backtracking across positions).
    bool monotonic = true;
    float prev_x = -1e9f;
    for (int i = 0; i <= end; ++i) {
        const yzk::Point cp = backend.caret_position(font, wrapped, wrap_w, i);
        if (cp.y > 0.0f) break;  // second line starts
        if (cp.x < prev_x) monotonic = false;
        prev_x = cp.x;
    }
    CHECK(monotonic);
}

static void test_tab_control_render() {
    VisualHarness h(420, 240);
    CHECK(h.init());

    yzk::TabControl tabs;
    tabs.set_bounds(yzk::RectF::make(0, 0, 420, 240));
    tabs.add_tab(yzk::utf::to_utf8(L"General"), new yzk::Label(yzk::utf::to_utf8(L"General settings here")));
    auto* about = new yzk::Label(yzk::utf::to_utf8(L"About this build - v0.3.0"));
    tabs.add_tab(yzk::utf::to_utf8(L"About"), about);
    auto* active = new yzk::Label(yzk::utf::to_utf8(L"Notifications page"));
    tabs.add_tab(yzk::utf::to_utf8(L"Notifications"), active);

    auto hash = h.render_hash(&tabs);
    check_or_record("tab_control", hash);
    CHECK(hash != 0);
    if (g_record_mode) h.save(fs::path(g_baseline_path).parent_path() / "tab_control.png", &tabs);

    // Switching pages changes the visible content: a fresh render must differ.
    tabs.set_selected_index(2);
    auto hash2 = h.render_hash(&tabs);
    CHECK(hash2 != 0);
    CHECK(hash2 != hash);
}

static void test_stack_messagebox_self_owns() {
    // Regression: a fire-and-forget stack MessageBox (as in cookbook_dialogs'
    // Quit handler) must outlive its own destruction — the close-animation tween
    // must not fire into a freed panel.
    VisualHarness h(400, 300);
    CHECK(h.init());

    yzk::Widget root;
    root.set_min_size(yzk::Size{400.0f, 300.0f});
    h.window().set_root(&root);

    {
        yzk::MessageBox m;
        m.show(h.window(), "Quit", "Quit the editor?", yzk::MessageBoxKind::YesNo);
        CHECK(m.is_open());
    }  // destroyed while still on screen

    // Pump the animation loop: a dangling tween would crash or corrupt here.
    for (int i = 0; i < 20; ++i) {
        Sleep(16);
        h.window().pump();
    }
    auto px = h.render(&root);
    CHECK(!px.empty());
}

// ===== Runner =====
int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--record") g_record_mode = true;
        if (std::string(argv[i]) == "--ascii-overlay") {
            debug_animated_overlay_ascii();
            return 0;
        }
    }

    // Set baseline path next to executable
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    g_baseline_path = yzk::utf::to_utf8(
        std::filesystem::path(exe).parent_path().append(L"visual_baseline.txt").wstring());

    load_baselines();

    std::printf("=== Visual Regression Tests ===\n");

    test_solid_background();
    test_label_centered();
    test_button_normal();
    test_stack_panel();
    test_box_styled();
    test_checkbox_unchecked();
    test_toggle_off();
    test_slider_50();
    test_progress_75();
    test_light_theme();
    test_grid_2x2();
    test_spinbox_default();
    test_combobox();
    test_flexbox_grow();
    test_dock_panel();
    test_radio_group();
    test_wrap_panel();
    test_scrollview();
    test_label_alignments();
    test_box_shadow();
    test_device_loss_recovery();
    test_message_box_render();
    test_message_box_long_message();
    test_label_rich_spans();
    test_d2d_text_hit_test();
    test_stack_messagebox_self_owns();
    test_tab_control_render();

    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);

    if (g_failures == 0) save_baselines();

    return g_failures > 0 ? 1 : 0;
}
