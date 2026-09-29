#include "uitraits.h"

#include <wx/font.h>
#include <wx/settings.h>
#include <wx/window.h>
#ifdef __WXMSW__
#include <windows.h>
#include <dwmapi.h>
#endif

namespace
{
// CJK-capable UI face. "Microsoft YaHei UI" covers Chinese, "Segoe UI"
// covers Latin; whichever the OS actually has installed is used, so mixed
// Chinese/Latin labels share one metric family instead of falling back to
// a monospaced or serif default.
const wxChar* kUiFaces[] = {
    wxT("Microsoft YaHei UI"),
    wxT("Microsoft YaHei"),
    wxT("Segoe UI"),
    wxT("PingFang SC"),
    wxT("Noto Sans CJK SC"),
    wxT("Helvetica Neue"),
};

// Scale a point size by the current display's DPI, so font sizes follow the
// same scaling rule as the layout metrics.
int ScaledPt(int ptSize)
{
    return wxWindowBase::FromDIP(ptSize, nullptr);
}
} // namespace

namespace UiTraits
{

wxFont UiFont(int ptSize, wxFontWeight weight)
{
    wxFont font(ScaledPt(ptSize), wxFONTFAMILY_SWISS,
                wxFONTSTYLE_NORMAL, weight);
    font.SetFaceName(wxString(kUiFaces[0]));
    // Try the preferred faces in order and keep the first one the OS
    // actually resolves.
    for (const wxChar* face : kUiFaces) {
        wxFont candidate = font;
        candidate.SetFaceName(wxString(face));
        if (candidate.IsOk() && candidate.GetFaceName() == wxString(face)) {
            font = candidate;
            break;
        }
    }
    return font;
}

wxFont UiFontBold(int ptSize)
{
    return UiFont(ptSize, wxFONTWEIGHT_BOLD);
}

Palette GetPalette(bool darkMode)
{
    Palette p;
    if (darkMode) {
        p.headerBg        = wxColour(0x1b, 0x1e, 0x24);
        p.headerFg        = wxColour(0xec, 0xef, 0xf3);
        p.headerMutedFg   = wxColour(0x8b, 0x93, 0x9e);
        p.divider         = wxColour(0x2a, 0x2e, 0x36);
        p.contentBgTop    = wxColour(0x1a, 0x1d, 0x22);
        p.contentBgBottom = wxColour(0x16, 0x18, 0x1c);
        p.cardBg          = wxColour(0x21, 0x24, 0x2a);
        p.cardBgHover     = wxColour(0x2a, 0x2e, 0x35);
        p.cardBorder      = wxColour(0x2c, 0x30, 0x37);
        p.textFg          = wxColour(0xe4, 0xe7, 0xeb);
        p.textMutedFg     = wxColour(0x93, 0x9a, 0xa4);
        p.accent          = wxColour(0x4d, 0xa3, 0xff);
        p.statusBg        = wxColour(0x14, 0x16, 0x1a);
        p.statusFg        = wxColour(0x9b, 0xa3, 0xad);
        p.addCardBg       = wxColour(0x1d, 0x1f, 0x24);
        p.addCardBorder   = wxColour(0x33, 0x37, 0x3e);
        p.addIconBg       = wxColour(0x26, 0x29, 0x2f);
        p.surfaceBg       = wxColour(0x1b, 0x1e, 0x23);
        p.accentSoft      = wxColour(0x22, 0x3a, 0x55);
    } else {
        p.headerBg        = wxColour(0xff, 0xff, 0xff);
        p.headerFg        = wxColour(0x1f, 0x23, 0x28);
        p.headerMutedFg   = wxColour(0x6b, 0x70, 0x76);
        p.divider         = wxColour(0xe6, 0xe8, 0xeb);
        p.contentBgTop    = wxColour(0xfa, 0xfb, 0xfc);
        p.contentBgBottom = wxColour(0xf4, 0xf6, 0xf8);
        p.cardBg          = *wxWHITE;
        p.cardBgHover     = wxColour(0xff, 0xff, 0xff);
        p.cardBorder      = wxColour(0xe3, 0xe6, 0xea);
        p.textFg          = wxColour(0x2b, 0x2f, 0x34);
        p.textMutedFg     = wxColour(0x76, 0x7c, 0x83);
        p.accent          = wxColour(0x1a, 0x73, 0xe8);
        p.statusBg        = wxColour(0xf7, 0xf8, 0xfa);
        p.statusFg        = wxColour(0x76, 0x7c, 0x83);
        p.addCardBg       = wxColour(0xf7, 0xf8, 0xfa);
        p.addCardBorder   = wxColour(0xdc, 0xe0, 0xe5);
        p.addIconBg       = wxColour(0xe9, 0xec, 0xf0);
        p.surfaceBg       = wxColour(0xff, 0xff, 0xff);
        p.accentSoft      = wxColour(0xe4, 0xee, 0xfc);
    }
    return p;
}

wxColour Shift(const wxColour& c, int delta)
{
    const auto ch = [delta](unsigned char v) {
        const int n = static_cast<int>(v) + delta;
        return static_cast<unsigned char>(n < 0 ? 0 : (n > 255 ? 255 : n));
    };
    return wxColour(ch(c.Red()), ch(c.Green()), ch(c.Blue()));
}

bool IsSystemDark()
{
#ifdef __WXMSW__
    // Windows 10/11: AppsUseLightTheme = 0 means dark, 1 means light
    DWORD value = 1, size = sizeof(value);
    LONG r = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme",
        RRF_RT_REG_DWORD, nullptr, &value, &size);
    if (r != ERROR_SUCCESS) value = 1;
    return (value == 0);
#else
    return wxSystemSettings::GetAppearance().IsDark();
#endif
}

void ApplyDarkFrame(wxWindow* w, bool dark)
{
#ifdef __WXMSW__
    if (!w)
        return;
    HWND hwnd = static_cast<HWND>(w->GetHWND());
    if (!hwnd)
        return;

    // 20 is DWMWA_USE_IMMERSIVE_DARK_MODE on Windows 10 2004+; build 19 is
    // the same attribute on the earlier builds. Set both so the title bar
    // follows the app on either.
    const BOOL useDark = dark ? TRUE : FALSE;
    if (FAILED(DwmSetWindowAttribute(hwnd, 20, &useDark, sizeof(useDark))))
        DwmSetWindowAttribute(hwnd, 19, &useDark, sizeof(useDark));
#endif
}

} // namespace UiTraits
