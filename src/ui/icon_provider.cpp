// Yuzuki Icon API: global provider service + built-in Phosphor provider.
#include <yuzuki/ui/icon.hpp>
#include <yuzuki/render/backend.hpp>
#include <yuzuki/core/encoding.hpp>

#include <windows.h>

namespace yzk {
namespace icon {
namespace {

class PhosphorIconProvider : public IconProvider {
public:
    const char* family() const override { return icon_family; }

    String glyph(IconId id) const override {
        String s;
        icon_detail::append_utf8(s, static_cast<u32>(id));
        return s;
    }

    bool register_resources(RenderBackend& backend) override {
        if (registered_) return true;
        const String path = ttf_path();
        if (path.empty()) return false;
        if (!backend.add_font_file(path)) return false;
        registered_ = true;
        return true;
    }

    void set_font_file(const String& path) {
        ttf_path_ = path;
        registered_ = false;
    }

private:
    // Default: Phosphor.ttf next to the executable. Set explicitly to override.
    String ttf_path() const {
        if (!ttf_path_.empty()) return ttf_path_;
        wchar_t exe[MAX_PATH] = {};
        const DWORD n = GetModuleFileNameW(nullptr, exe, MAX_PATH);
        if (n == 0 || n >= MAX_PATH) return {};
        std::wstring dir(exe, std::wcsrchr(exe, L'\\') + 1);
        return utf::to_utf8(dir + L"Phosphor.ttf");
    }

    String ttf_path_;
    bool registered_ = false;
};

std::unique_ptr<IconProvider>& global_provider() {
    static std::unique_ptr<IconProvider> instance;
    return instance;
}

}  // namespace

IconProvider& phosphor_provider() {
    static PhosphorIconProvider phosphor;
    return phosphor;
}

void set_phosphor_font_file(const String& path) {
    static_cast<PhosphorIconProvider&>(phosphor_provider()).set_font_file(path);
}

void set_provider(std::unique_ptr<IconProvider> provider) {
    global_provider() = std::move(provider);
}

IconProvider& provider() {
    std::unique_ptr<IconProvider>& slot = global_provider();
    if (!slot) slot = std::make_unique<PhosphorIconProvider>();
    return *slot;
}

}  // namespace icon
}  // namespace yzk