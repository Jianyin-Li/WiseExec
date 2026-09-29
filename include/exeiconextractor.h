#ifndef EXEICONEXTRACTOR_H
#define EXEICONEXTRACTOR_H

#include <wx/string.h>
#include <wx/bitmap.h>
#include <vector>

// Extracts the icon of an executable/binary file in a cross-platform way.
//
// Windows: ExtractIconEx() reads the icon group resource embedded in the PE
//          image (so it also works for files that are not registered).
// Other:    there is no portable API to read a PE resource, so we probe the
//          conventional side-car icon files and the XDG desktop entry, then
//          give up and let the caller fall back to the generated icon.
//
// All entry points return an invalid wxBitmap when nothing could be
// extracted; callers must check IsOk() before using the result.
namespace ExeIconExtractor
{
    // True when the path names something we are willing to try to extract an
    // icon from (an existing file with an executable-ish extension). On
    // Windows the extension test is skipped so that shortcuts/extension-less
    // binaries still work.
    bool IsExecutablePath(const wxString& path);

    // Extract the icon of the executable at `path`, scaled so that its
    // largest side is `size` pixels. Returns an invalid bitmap on failure.
    wxBitmap Extract(const wxString& path, int size = 64);

    // Extract the first usable icon found by trying each candidate in order.
    // Returns an invalid bitmap when every candidate fails.
    wxBitmap ExtractFirst(const std::vector<wxString>& candidates, int size = 64);
}

#endif // EXEICONEXTRACTOR_H
