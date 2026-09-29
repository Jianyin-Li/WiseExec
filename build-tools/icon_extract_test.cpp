// Standalone check for ExeIconExtractor against real executables.
// Enabled with -DBUILD_TESTS=ON; not part of the shipped application.
//
// Built as a GUI-subsystem binary because the statically linked wxWidgets
// build in this environment fails to load under the console subsystem. Such
// a binary has no attached stdout, so every result is also appended to
// `icon_extract_test.log` next to the executable.
#include "exeiconextractor.h"

#include <wx/init.h>
#include <wx/image.h>
#include <wx/filename.h>

#include <cstdarg>
#include <cstdio>
#include <string>

static FILE* g_log = nullptr;

static void Report(const char* fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    std::printf("%s", buf);
    if (g_log) {
        std::fputs(buf, g_log);
        std::fflush(g_log);
    }
}

static int g_fail = 0;

static void Check(const char* what, const wxString& path, bool expectOk)
{
    wxBitmap bmp = ExeIconExtractor::Extract(path, 64);
    const bool ok = bmp.IsOk();
    if (ok != expectOk) {
        Report("FAIL  %-34s expect=%d got=%d\n", what, expectOk ? 1 : 0, ok ? 1 : 0);
        ++g_fail;
        return;
    }
    if (ok) {
        wxImage img = bmp.ConvertToImage();
        Report("ok    %-34s %dx%d alpha=%d\n", what,
               img.GetWidth(), img.GetHeight(), img.HasAlpha() ? 1 : 0);
    } else {
        Report("ok    %-34s (no icon, as expected)\n", what);
    }
}

int main(int argc, char** argv)
{
    const wxString logPath =
        wxFileName(wxFileName(argv[0]).GetPath(), wxT("icon_extract_test.log")).GetFullPath();
    g_log = std::fopen(logPath.mb_str(), "wb");

    wxInitializer initializer(argc, argv);
    if (!initializer.IsOk()) {
        Report("FAIL  wxInitializer failed\n");
        return 2;
    }
    wxInitAllImageHandlers();

    // This binary itself carries an icon resource.
    Check("self (icon_extract_test.exe)", wxString::FromUTF8(argv[0]), true);

#ifdef __WXMSW__
    // System binaries with a real icon group.
    Check("notepad.exe", wxT("C:\\Windows\\System32\\notepad.exe"), true);
    Check("cmd.exe", wxT("C:\\Windows\\System32\\cmd.exe"), true);
#endif

    // Inputs that must never yield an icon.
    Check("missing file", wxT("C:\\nope\\missing.exe"), false);
    Check("empty path", wxEmptyString, false);
    Check("directory", wxT("C:\\Windows\\System32"), false);

    if (ExeIconExtractor::IsExecutablePath(wxT("C:\\Windows\\System32\\cmd.exe")) != true) {
        Report("FAIL  IsExecutablePath(existing exe) should be true\n");
        ++g_fail;
    }
    if (ExeIconExtractor::IsExecutablePath(wxT("C:\\nope\\missing.exe")) != false) {
        Report("FAIL  IsExecutablePath(missing) should be false\n");
        ++g_fail;
    }

    // ExtractFirst must skip unusable candidates and return the first hit.
    {
        std::vector<wxString> cands;
        cands.push_back(wxT("C:\\nope\\missing.exe"));
        cands.push_back(wxT("C:\\Windows\\System32\\notepad.exe"));
        if (!ExeIconExtractor::ExtractFirst(cands, 64).IsOk()) {
            Report("FAIL  ExtractFirst should skip the missing entry\n");
            ++g_fail;
        } else {
            Report("ok    ExtractFirst skipped missing entry\n");
        }
    }
    {
        std::vector<wxString> none;
        none.push_back(wxT("C:\\nope\\missing.exe"));
        if (ExeIconExtractor::ExtractFirst(none, 64).IsOk()) {
            Report("FAIL  ExtractFirst over all-missing should be invalid\n");
            ++g_fail;
        } else {
            Report("ok    ExtractFirst over all-missing is invalid\n");
        }
    }

    Report(g_fail == 0 ? "\nALL CHECKS PASSED\n" : "\n%d CHECK(S) FAILED\n", g_fail);
    if (g_log) std::fclose(g_log);
    return g_fail == 0 ? 0 : 1;
}
