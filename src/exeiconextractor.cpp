#include "exeiconextractor.h"

#include <wx/filename.h>
#include <wx/image.h>
#include <wx/wfstream.h>
#ifdef __WXMSW__
#include <wx/icon.h>
#include <windows.h>
#include <shellapi.h>
#endif

#include <algorithm>

namespace
{
// Trimmed copy of a string. wxString::Trim() is non-const, so a const
// reference cannot be trimmed in place.
wxString Trimmed(const wxString& value)
{
    wxString copy = value;
    return copy.Trim(true).Trim(false);
}

// Scale `bmp` so that its largest side is at most `size`, keeping alpha.
wxBitmap FitToBox(const wxBitmap& bmp, int size)
{
    if (!bmp.IsOk() || size <= 0)
        return wxNullBitmap;

    wxImage img = bmp.ConvertToImage();
    if (!img.IsOk())
        return wxNullBitmap;

    int w = img.GetWidth();
    int h = img.GetHeight();
    int largest = std::max(w, h);
    // Always normalise to exactly `size` on the longest side: callers draw
    // the result inside a fixed-size box, and a smaller bitmap would
    // otherwise be centre-cropped to a fraction of its own area.
    double scale = (largest > 0) ? static_cast<double>(size) / largest : 1.0;
    const int tw = std::max(1, static_cast<int>(w * scale));
    const int th = std::max(1, static_cast<int>(h * scale));
    if (tw != w || th != h)
        img.Rescale(tw, th, wxIMAGE_QUALITY_HIGH);

    // Build the bitmap with an alpha channel so that 32bpp icons (which carry
    // their transparency in the alpha channel) keep it instead of showing a
    // black box, as happens with the screen-depth DDB path.
    if (img.HasAlpha())
        return wxBitmap(img, 32);
    return wxBitmap(img);
}

#ifdef __WXMSW__

// Windows: pull the icon group resource straight out of the PE image.
// ExtractIconEx works on unregistered files, unlike the shell icon APIs.
wxBitmap ExtractWindows(const wxString& path, int size)
{
    if (!wxFileName::FileExists(path))
        return wxNullBitmap;

    wxString native = path;
    wchar_t* buf = const_cast<wchar_t*>(native.wx_str());

    // Ask how many icons the file carries first; 0 means it has no icon
    // resource at all and there is nothing to extract.
    if (ExtractIconEx(buf, -1, nullptr, nullptr, 0) == 0)
        return wxNullBitmap;

    HICON large = nullptr;
    HICON small = nullptr;
    // Ask for the system "large icon" metric so a 32x32-only resource is
    // still returned at its native size; the result is normalised by
    // FitToBox afterwards.
    if (ExtractIconEx(buf, 0, &large, &small, 1) == 0)
        return wxNullBitmap;

    // Prefer the large variant, which stays sharp after being scaled.
    HICON handle = large ? large : small;

    wxBitmap result;
    if (handle) {
        // The ctor takes ownership of the handle, so `handle` must not be
        // destroyed here; only the variant we did not hand over is.
        wxIcon icon;
        if (icon.CreateFromHICON(reinterpret_cast<WXHICON>(handle))) {
            // Copy the icon into a bitmap with an alpha channel, otherwise a
            // 32bpp icon renders its transparent pixels as opaque black.
            result = FitToBox(wxBitmap(icon, wxBitmapTransparency_Always), size);
        }
    }

    // Destroy the variants the wxIcon did not take over.
    if (large != handle && large) DestroyIcon(large);
    if (small != handle && small) DestroyIcon(small);
    return result;
}

#else // !__WXMSW__

// Non-Windows: try the conventional side-car icons that sit next to the
// binary, e.g. /usr/bin/gedit -> /usr/bin/gedit.png.
wxBitmap ExtractSideCar(const wxString& path, int size)
{
    static const wxChar* kExts[] = { wxT(".png"), wxT(".svg"), wxT(".xpm") };

    wxFileName fn(path);
    for (const wxChar* ext : kExts) {
        wxFileName candidate(fn.GetPath(wxPATH_UNIX), fn.GetName() + ext);
        if (wxFileName::FileExists(candidate)) {
            wxImage img(candidate.GetFullPath());
            if (img.IsOk())
                return FitToBox(wxBitmap(img), size);
        }
    }
    return wxNullBitmap;
}

// Non-Windows: read the Icon= key of the XDG desktop entry matching the
// binary name, honouring the freedesktop icon-theme lookup loosely (absolute
// paths and bare icon names both work).
wxBitmap ExtractDesktopEntry(const wxString& path, int size)
{
    wxFileName fn(path);
    const wxString name = fn.GetName();
    if (name.IsEmpty())
        return wxNullBitmap;

    static const wxChar* kDirs[] = {
        wxT("/usr/share/applications"),
        wxT("/usr/local/share/applications"),
        wxT("/var/lib/flatpak/exports/share/applications"),
        wxT("~/.local/share/applications"),
    };

    for (const wxChar* dir : kDirs) {
        wxFileName entry(dir, name + wxT(".desktop"));
        entry.Normalize(wxPATH_NORM_ENV | wxPATH_NORM_DOTS);
        if (!wxFileName::FileExists(entry))
            continue;

        wxFFile file(entry.GetFullPath(), wxT("rb"));
        if (!file.IsOpened())
            continue;

        wxString content;
        file.ReadAll(&content, wxConvUTF8);

        // Locate the Icon= entry inside the [Desktop Entry] group.
        wxString iconValue;
        bool inGroup = false;
        content.Replace(wxT("\r\n"), wxT("\n"));
        content.Replace(wxT("\r"), wxT("\n"));
        for (const wxString& raw : wxSplit(content, wxT('\n'))) {
            const wxString line = raw.Trim(true).Trim(false);
            if (line.IsEmpty() || line[0] == '#')
                continue;
            if (line[0] == '[') {
                inGroup = (line == wxT("[Desktop Entry]"));
                continue;
            }
            if (!inGroup)
                continue;
            const size_t eq = line.find(wxT('='));
            if (eq == wxString::npos)
                continue;
            if (line.Left(eq).Trim(true).Trim(false) != wxT("Icon"))
                continue;
            iconValue = line.Mid(eq + 1).Trim(true).Trim(false);
            break;
        }

        if (iconValue.IsEmpty())
            continue;

        // An absolute or resolvable path is used directly.
        if (wxFileName::FileExists(wxFileName(iconValue))) {
            wxImage img(wxFileName(iconValue).GetFullPath());
            if (img.IsOk())
                return FitToBox(wxBitmap(img), size);
        }

        // Bare icon name: probe the usual theme locations.
        static const wxChar* kIconDirs[] = {
            wxT("/usr/share/icons/hicolor"),
            wxT("/usr/share/pixmaps"),
            wxT("/usr/share/icons"),
        };
        static const int kSizes[] = { 48, 64, 32, 128, 256, 16 };
        static const wxChar* kExts[] = { wxT(".png"), wxT(".svg") };
        for (const wxChar* base : kIconDirs) {
            for (int s : kSizes) {
                for (const wxChar* ext : kExts) {
                    wxString sub = wxString::Format(wxT("%s/%dx%d/apps/%s%s"),
                                                    base, s, s, iconValue, ext);
                    if (wxFileName::FileExists(sub)) {
                        wxImage img(sub);
                        if (img.IsOk())
                            return FitToBox(wxBitmap(img), size);
                    }
                }
            }
        }
    }
    return wxNullBitmap;
}

#endif // !__WXMSW__
} // namespace

namespace ExeIconExtractor
{

bool IsExecutablePath(const wxString& path)
{
    const wxString trimmed = Trimmed(path);
    if (trimmed.IsEmpty() || !wxFileName::FileExists(trimmed))
        return false;

    wxFileName fn(trimmed);
    if (fn.IsDir())
        return false;

#ifdef __WXMSW__
    // ExtractIconEx can read any PE file, so do not filter by extension:
    // the user may point at a file with an unusual name.
    (void)0;
    return true;
#else
    static const wxChar* kExts[] = {
        wxT(""), wxT(".exe"), wxT(".bat"), wxT(".cmd"), wxT(".com"),
        wxT(".sh"), wxT(".py"), wxT(".pl"), wxT(".rb"), wxT(".js"),
        wxT(".AppImage"), wxT(".appimage"),
    };
    const wxString ext = fn.GetExt().MakeLower();
    for (const wxChar* candidate : kExts) {
        if (ext == candidate)
            return true;
    }
    // Registered as a program by the desktop database.
    if (fn.GetName().IsEmpty())
        return false;
    return true;
#endif
}

wxBitmap Extract(const wxString& path, int size)
{
    const wxString trimmed = Trimmed(path);
    if (!IsExecutablePath(trimmed))
        return wxNullBitmap;

#ifdef __WXMSW__
    return ExtractWindows(trimmed, size);
#else
    wxBitmap bmp = ExtractSideCar(trimmed, size);
    if (bmp.IsOk())
        return bmp;
    bmp = ExtractDesktopEntry(trimmed, size);
    if (bmp.IsOk())
        return bmp;
    return wxNullBitmap;
#endif
}

wxBitmap ExtractFirst(const std::vector<wxString>& candidates, int size)
{
    for (const wxString& candidate : candidates) {
        wxBitmap bmp = Extract(candidate, size);
        if (bmp.IsOk())
            return bmp;
    }
    return wxNullBitmap;
}

} // namespace ExeIconExtractor
