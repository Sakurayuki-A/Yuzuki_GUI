#include "app.hpp"

#include <yuzuki/yuzuki.hpp>
#include <yuzuki/ui/icon.hpp>
#include <yuzuki/ui/animation.hpp>

#include <windows.h>
#include <shellapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#endif

#include <filesystem>
#include <vector>
#include <algorithm>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace fs = std::filesystem;

namespace yzk {
namespace {

using Path = fs::path;

// ---------------------------------------------------------------------
// Row model
// ---------------------------------------------------------------------

struct Entry {
    String name;  // UTF-8 display name
    bool is_dir = false;
    IconId icon = IconId::File;
    Path full;  // absolute path (browse = current/name, search = absolute)
};

IconId icon_for(const String& ext) {
    if (ext.empty()) return IconId::File;
    if (ext == ".txt" || ext == ".md" || ext == ".log" || ext == ".csv") return IconId::FileTxt;
    if (ext == ".cpp" || ext == ".hpp" || ext == ".h" || ext == ".c" || ext == ".cc")
        return IconId::FileCpp;
    if (ext == ".js" || ext == ".mjs" || ext == ".jsx") return IconId::FileJs;
    if (ext == ".ts" || ext == ".tsx") return IconId::FileTs;
    if (ext == ".py") return IconId::FilePy;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".gif" ||
        ext == ".svg" || ext == ".bmp" || ext == ".webp" || ext == ".ico")
        return IconId::FileImage;
    if (ext == ".pdf") return IconId::FilePdf;
    if (ext == ".zip" || ext == ".rar" || ext == ".7z" || ext == ".tar" || ext == ".gz")
        return IconId::FileZip;
    if (ext == ".html" || ext == ".htm" || ext == ".css") return IconId::FileHtml;
    if (ext == ".doc" || ext == ".docx") return IconId::FileDoc;
    if (ext == ".xls" || ext == ".xlsx") return IconId::FileXls;
    return IconId::File;
}

class DirSource : public ListView::DataSource {
public:
    Path current;
    std::vector<Entry> rows;      // all entries of current dir (sorted)
    std::vector<Entry> visible;   // filtered/search view shown by the list (UI thread)
    bool searching = false;       // true while a global recursive search is active
    bool search_done = false;     // recursive walk finished (no more rows to stream)
    i32 result_cap = 2000;        // safety cap on streamed results

    ~DirSource() { stop_search(); }

    void load(const Path& dir) {
        stop_search();
        current = dir;
        rows.clear();
        visible.clear();
        searching = false;
        search_done = true;
        if (!fs::exists(dir) || !fs::is_directory(dir)) return;
        std::error_code ec;
        for (const auto& it : fs::directory_iterator(dir, ec)) {
            if (ec) break;
            const fs::path p = it.path();
            const String name = p.filename().u8string();
            const bool is_dir = it.is_directory(ec);
            rows.push_back(Entry{name, is_dir,
                                 is_dir ? IconId::Folder : icon_for(p.extension().string()),
                                 p});
        }
        std::sort(rows.begin(), rows.end(), [](const Entry& a, const Entry& b) {
            if (a.is_dir != b.is_dir) return a.is_dir;  // directories first
            return a.name < b.name;
        });
        visible = rows;
    }

    // Global recursive search starting at current. A dedicated worker thread walks
    // the tree; matches are queued (never touch the widget tree). The UI thread
    // calls drain_results() once a frame to deliver them in batches.
    void begin_search(const String& needle) {
        stop_search();
        filter_ = needle;
        visible.clear();
        searching = true;
        search_done = false;
        match_count_ = 0;
        active_gen_.store(++generation_, std::memory_order_relaxed);
        const u32 gen = generation_;
        const Path root = current;
        worker_ = std::thread([this, gen, root, needle]() {
            search_worker(gen, root, needle);
        });
    }

    // Worker: walks root with its own iterator, appends matches to the shared queue,
    // stopping when cancelled (generation changed) or the cap is reached.
    void search_worker(u32 gen, const Path root, const String needle) {
        std::error_code ec;
        fs::recursive_directory_iterator it(
            root, fs::directory_options::skip_permission_denied, ec);
        std::wstring ws = utf::to_wide(needle);
        std::transform(ws.begin(), ws.end(), ws.begin(), ::towlower);

        i32 found = 0;
        bool done = false;
        while (!done) {
            std::vector<Entry> batch;
            batch.reserve(512);
            for (i32 i = 0; i < 512; ++i) {
                if (cancelled(gen) || found >= result_cap) { done = true; break; }
                if (it == fs::recursive_directory_iterator()) { done = true; break; }
                const fs::directory_entry& de = *it;
                const fs::path p = de.path();
                const bool is_dir = de.is_directory(ec);
                if (!p.is_relative()) {
                    std::wstring w = utf::to_wide(p.filename().u8string());
                    std::transform(w.begin(), w.end(), w.begin(), ::towlower);
                    if (w.find(ws) != std::wstring::npos) {
                        batch.push_back(Entry{p.filename().u8string(), is_dir,
                                              is_dir ? IconId::Folder
                                                     : icon_for(p.extension().string()),
                                              p});
                        ++found;
                    }
                }
                it.increment(ec);
                if (ec) { done = true; break; }
            }
            if (!batch.empty()) push_batch(gen, std::move(batch));
            if (found >= result_cap) done = true;
        }
        std::lock_guard<std::mutex> lock(queue_mutex_);
        search_done_shared_ = true;
    }

    // UI thread: moves queued matches into visible; returns true if anything
    // changed (new rows delivered, or the search reached its end).
    bool drain_results(i32 budget) {
        (void)budget;
        if (!searching || search_done) return false;
        bool changed = false;
        const auto ready = pop_batch(active_gen_.load(std::memory_order_relaxed));
        if (!ready.empty()) {
            visible.insert(visible.end(), ready.begin(), ready.end());
            match_count_ = static_cast<i32>(visible.size());
            changed = true;
        }
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            if (search_done_shared_) {
                queue_.clear();
                search_done = true;
                searching = false;
                changed = true;
            }
        }
        return changed;
    }

    void stop_search() {
        active_gen_.store(++generation_, std::memory_order_relaxed);
        if (worker_.joinable()) worker_.join();
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.clear();
        search_done_shared_ = false;
    }

    void set_filter(const String& needle) {
        filter_ = needle;
        if (needle.empty()) {
            searching = false;
            search_done = true;
            stop_search();
            visible = rows;
        } else {
            begin_search(needle);
        }
    }
    const String& filter() const { return filter_; }

    Path path_for(i32 index) const {
        if (index < 0 || index >= static_cast<i32>(visible.size())) return {};
        const Entry& e = visible[static_cast<size_t>(index)];
        return e.full.empty() ? current / e.name : e.full;
    }

    i32 count() const override { return static_cast<i32>(visible.size()); }
    String text_at(i32 index) const override {
        if (index < 0 || index >= static_cast<i32>(visible.size())) return {};
        return visible[static_cast<size_t>(index)].name;
    }

private:
    bool cancelled(u32 gen) const {
        return active_gen_.load(std::memory_order_relaxed) != gen;
    }

    void push_batch(u32 gen, std::vector<Entry> batch) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        queue_.emplace_back(std::make_pair(gen, std::move(batch)));
    }

    // UI thread: takes the next batch (old-generation stragglers are dropped).
    std::vector<Entry> pop_batch(u32 gen) const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        for (auto it = queue_.begin(); it != queue_.end(); ++it) {
            if (it->first == gen) {
                std::vector<Entry> out = std::move(it->second);
                queue_.erase(it);
                return out;
            }
            queue_.erase(it);  // stale generation: discard
            break;
        }
        return {};
    }

    String filter_;
    std::thread worker_;
    std::atomic<u32> generation_{0};
    std::atomic<u32> active_gen_{0};
    mutable std::mutex queue_mutex_;
    mutable std::vector<std::pair<u32, std::vector<Entry>>> queue_;
    bool search_done_shared_ = false;
    i32 match_count_ = 0;
};

// Row painter: folder/file icon + name.
class DirRowDelegate : public ListView::RowDelegate {
public:
    DirSource& src;
    explicit DirRowDelegate(DirSource& s) : src(s) {}

    void draw(ListView&, PaintContext& ctx, i32 index, const RectF& r) override {
        const Theme& th = Theme::get();
        if (index < 0 || index >= static_cast<i32>(src.visible.size())) return;
        const Entry& e = src.visible[static_cast<size_t>(index)];

        const RectF ic = RectF::make(r.left + 8.0f, r.top + 4.0f, 20.0f, r.height() - 8.0f);
        const Color c = e.is_dir ? th.accent : th.text_secondary;
        ctx.draw_icon(e.icon, ic, c, 16.0f);

        const RectF tr = RectF::make(r.left + 34.0f, r.top, r.width() - 42.0f, r.height());
        ctx.draw_text(e.name, tr, th.text, TextAlignH::Left, TextAlignV::Center);
    }
};

// ListView that reports the right-clicked row so the app can open a context
// menu for it (the base ListView routes right-down through its selection path).
class ExplorerList : public ListView {
public:
    std::function<void(f32 x, f32 y, i32 row)> on_right_click;
    std::function<void(const std::vector<String>&)> on_drop_files;
    std::function<void()> on_near_bottom;
    void on_event(Event& e) override {
        ListView::on_event(e);
        if (e.type == EventType::MouseDown && (e.data.mouse.buttons & MouseButton_Right)) {
            const RectF g = global_bounds();
            const i32 row = row_from_y(e.data.mouse.y - g.top);
            if (row >= 0 && row < count() && on_right_click)
                on_right_click(e.data.mouse.x, e.data.mouse.y, row);
            e.consumed = true;
        } else if (e.type == EventType::DropFiles && e.data.drop.files && on_drop_files) {
            on_drop_files(*e.data.drop.files);
            e.consumed = true;
        } else if (on_near_bottom &&
                   (e.type == EventType::Wheel || e.type == EventType::MouseMove)) {
            if (near_bottom()) on_near_bottom();
        }
    }

    bool near_bottom() const {
        const f32 content_h = static_cast<f32>(count()) * row_height();
        if (content_h <= bounds_.height()) return true;
        return scroll_y() + bounds_.height() >= content_h - 4.0f * row_height();
    }

private:
    i32 row_from_y(f32 y) const {
        const f32 cy = y + scroll_y();
        if (cy < 0.0f) return -1;
        const i32 row = static_cast<i32>(cy / row_height());
        return row >= count() ? -1 : row;
    }
};

void copy_to_clipboard(const String& text) {
    if (!OpenClipboard(nullptr)) return;
    EmptyClipboard();
    std::wstring w = utf::to_wide(text);
    const size_t bytes = (w.size() + 1) * sizeof(wchar_t);
    HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (h) {
        if (void* dst = GlobalLock(h)) {
            memcpy(dst, w.c_str(), bytes);
            GlobalUnlock(h);
            SetClipboardData(CF_UNICODETEXT, h);
        }
    }
    CloseClipboard();
}

std::vector<Path> list_fixed_drives() {
    std::vector<Path> out;
    DWORD mask = GetLogicalDrives();
    for (char letter = 'A'; letter <= 'Z'; ++letter) {
        if (!(mask & (static_cast<DWORD>(1) << (letter - 'A')))) continue;
        const std::wstring root = (std::wstring(1, letter) + L":\\");
        if (GetDriveTypeW(root.c_str()) == DRIVE_FIXED) out.push_back(fs::path(root));
    }
    return out;
}

// ---------------------------------------------------------------------
// App state
// ---------------------------------------------------------------------

struct ExplorerState {
    DirSource source;
    DirRowDelegate delegate{source};
    ExplorerList* list = nullptr;
    TextBox* path_box = nullptr;
    TextBox* search_box = nullptr;
    Label* status = nullptr;
    std::function<void(const Path&)> open_path;  // directory -> navigate, file -> open
    AnimationSystem::FrameToken frame_token = 0;
};

void update_status(ExplorerState* st) {
    if (!st->status) return;
    char buf[96];
    if (st->source.searching) {
        if (st->source.search_done)
            snprintf(buf, sizeof(buf), "%d matches",
                     st->source.count());
        else
            snprintf(buf, sizeof(buf), "%d matches, keep scrolling...",
                     st->source.count());
    } else {
        snprintf(buf, sizeof(buf), "%d items", st->source.count());
    }
    st->status->set_text(buf);
}

void render_dir(ExplorerState* st, const Path& dir) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return;
    if (!st->search_box->text().empty()) st->search_box->set_text("");
    st->source.load(dir);
    st->path_box->set_text(dir.u8string());
    update_status(st);
    st->list->set_selected(-1);
    st->list->set_scroll_y(0.0f);
    st->list->invalidate();
}

}  // namespace

// ---------------------------------------------------------------------
// App shell
// ---------------------------------------------------------------------

Widget* make_explorer(Window& win) {
    const Theme& th = Theme::get();
    auto* st = new ExplorerState;

    auto* root = new DockPanel;

    // ---- Sidebar ----
    auto* sidebar = new Box;
    sidebar->bg_role(ThemeRole::Surface).set_min_size(Size{168.0f, 0.0f});
    root->dock(sidebar, Dock::Left);

    auto* side = new StackPanel(Orientation::Vertical);
    side->set_spacing(2.0f).set_padding(10.0f);
    sidebar->append_child(side);

    auto* brand_row = new StackPanel(Orientation::Horizontal);
    brand_row->set_spacing(4.0f).set_fill(true).set_margin(Margins{0, 0, 0, 10});
    side->append_child(brand_row);

    auto* brand = new Label("Yuzuki");
    brand->bold(true).set_align(TextAlignH::Left, TextAlignV::Center)
        .set_text_color(th.accent).set_flex_grow(1.0f);
    brand_row->append_child(brand);

    // Refresh aligned to the right of the brand title.
    auto* refresh = new Button("");
    refresh->set_accent(false).set_icon(IconId::ArrowClockwise).set_min_width(0.0f);
    refresh->on_click([st]() { st->open_path(st->source.current); });
    brand_row->append_child(refresh);

    // open_path wires navigation + file-open; DirSource::load is the only source of truth.
    st->open_path = [st, &win](const Path& p) {
        std::error_code ec;
        if (fs::is_directory(p, ec)) {
            render_dir(st, p);
        } else if (!p.empty()) {
            ShellExecuteW(nullptr, L"open", utf::to_wide(p.u8string()).c_str(), nullptr, nullptr,
                          SW_SHOWNORMAL);
        }
    };

    // Quick-access rows.
    const Path home = fs::path(getenv("USERPROFILE") ? getenv("USERPROFILE") : "C:\\");
    struct QuickLink {
        const char* name;
        Path target;
    };
    const std::vector<QuickLink> quick_links = {
        {"Home", home},
        {"Desktop", home / "Desktop"},
        {"Documents", home / "Documents"},
        {"Pictures", home / "Pictures"},
        {"Music", home / "Music"},
    };
    for (const auto& ql : quick_links) {
        auto* li = new Button(ql.name);
        li->set_accent(false);
        li->on_click([st, target = ql.target]() { st->open_path(target); });
        side->append_child(li);
    }

    // ---- This PC with expandable drive list ----
    std::vector<Button*> drive_rows;
    Button* this_pc = nullptr;
    {
        auto* node = new StackPanel(Orientation::Vertical);
        node->set_spacing(2.0f).set_margin(Margins{0, 10, 0, 0});
        side->append_child(node);

        this_pc = new Button("");
        this_pc->set_accent(false).set_icon(IconId::CaretRight).set_min_width(0.0f);
        this_pc->set_text(" This PC");
        node->append_child(this_pc);

        auto* drives = new StackPanel(Orientation::Vertical);
        drives->set_spacing(2.0f);
        drives->set_visible(false);
        node->append_child(drives);

        for (const Path& d : list_fixed_drives()) {
            auto* dr = new Button(d.root_name().string().empty()
                                      ? d.string()
                                      : d.root_name().string());
            dr->set_accent(false).set_min_width(0.0f);
            dr->set_margin(Margins{10, 0, 0, 0});
            dr->on_click([st, target = d]() { st->open_path(target); });
            drives->append_child(dr);
            drive_rows.push_back(dr);
        }

        this_pc->on_click([drives, this_pc]() {
            const bool show = !drives->visible();
            drives->set_visible(show);
            this_pc->set_icon(show ? IconId::CaretDown : IconId::CaretRight);
            if (Widget* host = this_pc->parent()) host->invalidate();
        });
    }
    (void)drive_rows;

    // ---- Top bar ----
    auto* top = new StackPanel(Orientation::Horizontal);
    top->set_spacing(8.0f);
    top->set_padding(12.0f);
    top->set_fill(true);
    root->dock(top, Dock::Top);

    auto* up = new Button("");
    up->set_icon(IconId::ArrowUp).set_min_width(0.0f);
    up->on_click([st]() {
        const Path p = st->source.current.parent_path();
        if (!p.empty() && p != st->source.current) st->open_path(p);
    });
    top->append_child(up);

    // Editable path: type any directory and press Enter to navigate.
    st->path_box = new TextBox("");
    st->path_box->set_placeholder("Path").set_min_width(160.0f).set_flex_grow(1.0f);
    st->path_box->set_flex_grow(1.0f);
    st->path_box->on_commit([st]() {
        if (!st->search_box->text().empty()) {
            st->search_box->set_text("");
            st->source.set_filter("");
        }
        st->open_path(fs::u8path(st->path_box->text()));
    });
    top->append_child(st->path_box);

    // Browse: native open dialog picks an item; the explorer navigates to its
    // folder so browsing a tree still feels native (open_file_dialog is the
    // framework wrapper around IFileOpenDialog).
    auto* browse = new Button("");
    browse->set_icon(IconId::FolderOpen).set_min_width(0.0f);
    browse->on_click([st]() {
        FileDialogOptions opts;
        opts.filters = {{"All files", "*.*"}};
        const auto paths = open_file_dialog(nullptr, opts);
        if (paths.empty()) return;
        Path picked = fs::u8path(paths[0]);
        const Path target = fs::is_directory(picked) ? picked : picked.parent_path();
        if (!target.empty()) st->open_path(target);
    });
    top->append_child(browse);

    // Global recursive search (walks the whole tree under the current folder).
    // Worker thread scans; a per-frame callback on the UI thread drains batches.
    st->search_box = new TextBox("");
    st->search_box->set_placeholder("Filter").set_min_width(180.0f);
    st->search_box->on_commit([st]() {
        st->source.set_filter(st->search_box->text());
        st->list->set_selected(-1);
        st->list->set_scroll_y(0.0f);
        st->list->invalidate();
        update_status(st);
    });
    top->append_child(st->search_box);

    // ---- File list ----
    auto* lv = new ExplorerList;
    lv->set_row_height(30.0f).set_data_source(&st->source).set_row_delegate(&st->delegate)
        .set_focusable(true);
    lv->on_activate([st](i32 index) { st->open_path(st->source.path_for(index)); });
    // The worker thread streams results into a queue; drain them on the UI thread
    // each frame and refresh the list. Few results visible -> scroll appears to load
    // as the search finishes.
    st->frame_token = AnimationSystem::instance().on_frame([st](f32) {
        if (!st->source.searching || st->source.search_done) return;
        // Drain whatever the worker streamed since the last frame. Invalidate
        // unconditionally while searching: the pump only calls frame callbacks when
        // needs_paint_ is set, so this drives the render loop until the search ends.
        st->source.drain_results(256);
        st->list->invalidate();
        update_status(st);
    });
    root->dock(lv, Dock::Fill);
    st->list = lv;

    // ---- Context menu (right-click row) ----
    auto* menu = new ContextMenu;
    menu->add_item("Open", [st]() { st->open_path(st->source.path_for(st->list->selected())); });
    menu->add_item("Copy Path", [st]() {
        const Path p = st->source.path_for(st->list->selected());
        if (!p.empty()) copy_to_clipboard(p.u8string());
    });
    menu->add_separator();
    menu->add_item("Delete", [st]() {
        const Path p = st->source.path_for(st->list->selected());
        if (p.empty()) return;
        std::error_code ec;
        if (fs::is_directory(p, ec)) fs::remove_all(p, ec);
        else fs::remove(p, ec);
        st->source.load(st->source.current);
        render_dir(st, st->source.current);
    });
    win.set_context_menu(menu);

    lv->on_right_click = [st, menu, &win](f32 x, f32 y, i32 row) {
        st->list->set_selected(row);
        menu->open(win, x, y);
    };
    // Drag a file/folder onto the list: navigate to a dropped directory, or open
    // the first dropped file.
    lv->on_drop_files = [st](const std::vector<String>& files) {
        for (const String& f : files) {
            const Path p = fs::u8path(f);
            std::error_code ec;
            if (fs::is_directory(p, ec)) {
                st->open_path(p);
                return;
            }
        }
        if (!files.empty()) st->open_path(fs::u8path(files[0]));
    };

    // ---- Status bar ----
    auto* status_bar = new Box;
    status_bar->bg_role(ThemeRole::SurfaceContainer);
    root->dock(status_bar, Dock::Bottom);

    st->status = new Label("");
    st->status->text_role(TextRole::Secondary)
        .set_align(TextAlignH::Left, TextAlignV::Center).set_margin(Margins{12, 0, 4, 0});
    status_bar->append_child(st->status);

    // ---- Start at Home ----
    render_dir(st, home);

    return root;
}

}  // namespace yzk