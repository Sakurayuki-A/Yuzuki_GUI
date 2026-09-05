#pragma once
#include <yuzuki/core/types.hpp>
#include <memory>

// Icon system (Yuzuki Icon API): a global Icon Provider abstraction.
//
// Controls only speak IconId — the provider owns every backend detail (font file
// location, load timing, font family name, DirectWrite FontFace, glyph codepoint).
// The default provider renders Phosphor icon font (MIT license); swap providers
// (Material Symbols, Font Awesome, SVG…) without touching Button/Menu/TreeView.
namespace yzk {

class RenderBackend;

enum class IconId : u32;

constexpr const char* icon_family = "Phosphor";

namespace icon {

class IconProvider {
public:
    virtual ~IconProvider() = default;
    // Font family name the provider's glyphs are drawn with.
    virtual const char* family() const = 0;
    // IconId -> UTF-8 glyph string (drawn with family()).
    virtual String glyph(IconId id) const = 0;
    // Idempotent: registers the provider's font files on the backend (called by
    // PaintContext::draw_icon on each first-use per backend). Returns false on failure.
    virtual bool register_resources(RenderBackend& backend) = 0;
};

// Built-in Phosphor icon font provider — the default. Locates Phosphor.ttf next to
// the executable; override the path via icon::set_phosphor_font_file() before first draw.
IconProvider& phosphor_provider();
// Optional: point the Phosphor provider at a custom TTF path.
IconProvider& set_phosphor_font_file(const String& path);

// Installs the app-wide icon provider (default: Phosphor). Yuzuki comes with a
// built-in PhosphorIconProvider; call set_provider to swap the icon backend.
// Takes ownership of the provider; passing nullptr restores the default.
IconProvider& set_provider(std::unique_ptr<IconProvider> provider);
IconProvider& provider();

}  // namespace icon

enum class IconId : u32 {
    None = 0,
    Plus = 0xE3D4,
    CaretDown = 0xE136,
    CaretUp = 0xE13C,
    CaretRight = 0xE13A,
    CaretLeft = 0xE138,
    Check = 0xE182,
    Terminal = 0xE47E,
    Code = 0xE1BC,
    MagnifyingGlass = 0xE30C,
    Gear = 0xE270,
    PaperPlane = 0xE394,
    PaperPlaneTilt = 0xE398,
    Chat = 0xE15C,
    ChatCircle = 0xE168,
    X = 0xE4F6,
    DotsThree = 0xE1FE,
    Copy = 0xE1CA,
    FileCode = 0xE914,
    Question = 0xE3E8,
    ArrowDown = 0xE03E,
    ArrowUp = 0xE08E,
    ArrowRight = 0xE06C,
    List = 0xE2F0,
    Trash = 0xE4A6,
    PencilSimple = 0xE3B4,
    User = 0xE4C2,
    Users = 0xE4D6,
    BookOpen = 0xE0E6,
    Stack = 0xE466,
    WarningCircle = 0xE4E2,
    Info = 0xE2CE,
    ArrowClockwise = 0xE036,
    Clock = 0xE19A,
    CalendarDots = 0xE7B4,
    Camera = 0xE10E,
    Calendar = 0xE108,
    Bell = 0xE0CE,
    BellRinging = 0xE5E8,
    Bookmark = 0xE0E8,
    Briefcase = 0xE0EE,
    Browser = 0xE0F4,
    Buildings = 0xE102,
    Calculator = 0xE538,
    CalendarBlank = 0xE10A,
    CalendarCheck = 0xE712,
    CalendarX = 0xE10C,
    ChartBar = 0xE150,
    ChartLine = 0xE154,
    ChartPie = 0xE158,
    ChatCircleText = 0xE16E,
    ChatText = 0xE17A,
    CheckCircle = 0xE184,
    CheckSquare = 0xE186,
    Clipboard = 0xE196,
    ClipboardText = 0xE198,
    Cloud = 0xE1AA,
    CloudArrowDown = 0xE1AC,
    CloudCheck = 0xE1B0,
    Command = 0xE1C4,
    CopySimple = 0xE1CC,
    Cpu = 0xE610,
    Desktop = 0xE560,
    DeviceMobile = 0xE1E0,
    Download = 0xE20A,
    DownloadSimple = 0xE20C,
    Envelope = 0xE214,
    EnvelopeOpen = 0xE216,
    Eye = 0xE220,
    EyeClosed = 0xE222,
    File = 0xE230,
    FileCpp = 0xEB2E,
    FileCss = 0xEB34,
    FileDoc = 0xEB1E,
    FileHtml = 0xEB38,
    FileImage = 0xEA24,
    FileJs = 0xEB24,
    FilePdf = 0xE702,
    FilePng = 0xEB18,
    FilePy = 0xEB2C,
    FileText = 0xE23A,
    FileTs = 0xEB26,
    FileTxt = 0xEB32,
    FileXls = 0xEB22,
    FileZip = 0xE958,
    Fire = 0xE242,
    Flag = 0xE244,
    FloppyDisk = 0xE248,
    Folder = 0xE24A,
    FolderMinus = 0xE254,
    FolderOpen = 0xE256,
    FolderPlus = 0xE258,
    FolderSimple = 0xE25A,
    Gift = 0xE276,
    GithubLogo = 0xE576,
    Globe = 0xE288,
    GraduationCap = 0xE62C,
    Heart = 0xE2A8,
    HeartStraight = 0xE2AA,
    Hourglass = 0xE2B2,
    House = 0xE2C2,
};

namespace icon_detail {

inline void append_utf8(String& out, u32 cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

}  // namespace icon_detail

// Icon ID -> UTF-8 glyph string (for draw_text). Backed by the global provider;
// kept as a light alias so raw canvas drawing can stay one-liners, but controls
// should prefer PaintContext::draw_icon.
inline String icon_glyph(IconId id) { return icon::provider().glyph(id); }

}  // namespace yzk