#include <yuzuki/ui/clipboard.hpp>
#include <yuzuki/core/encoding.hpp>

#include <cstring>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace yzk {
namespace clipboard {

bool has_text() {
    return IsClipboardFormatAvailable(CF_UNICODETEXT) != FALSE;
}

bool set_text(const String& text) {
    const WString wide = utf::to_wide(text);
    if (wide.empty()) return false;
    if (!OpenClipboard(nullptr)) return false;
    EmptyClipboard();
    const SIZE_T bytes = (wide.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    bool ok = false;
    if (mem) {
        void* data = GlobalLock(mem);
        if (data) {
            std::memcpy(data, wide.c_str(), bytes);
            GlobalUnlock(mem);
            ok = SetClipboardData(CF_UNICODETEXT, mem) != nullptr;
        } else {
            GlobalFree(mem);
        }
    }
    CloseClipboard();
    return ok;
}

String get_text() {
    if (!OpenClipboard(nullptr)) return String();
    HANDLE mem = GetClipboardData(CF_UNICODETEXT);
    String result;
    if (mem) {
        const wchar_t* data = static_cast<const wchar_t*>(GlobalLock(mem));
        if (data) {
            result = utf::to_utf8(WString(data));
            GlobalUnlock(mem);
        }
    }
    CloseClipboard();
    return result;
}

}  // namespace clipboard
}  // namespace yzk