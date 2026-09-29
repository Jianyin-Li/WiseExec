#ifndef UITRAITS_H
#define UITRAITS_H

#include <wx/colour.h>
#include <wx/font.h>
#include <wx/string.h>
#include <wx/window.h>

// Shared look-and-feel helpers so the header bar, the icon grid, the dialogs
// and the language selector all draw from one palette instead of each file
// hard-coding its own colours.
namespace UiTraits
{
    // The UI font. wxFONTFAMILY_SWISS without an explicit face falls back to a
    // monospaced default on some Windows setups, which makes mixed
    // Chinese/Latin labels look ragged; asking for the system UI face (and
    // its CJK companion for Chinese text) keeps the metrics consistent.
    wxFont UiFont(int ptSize, wxFontWeight weight = wxFONTWEIGHT_NORMAL);
    wxFont UiFontBold(int ptSize);

    // Colours, resolved for the given theme. Centralising them keeps the dark
    // and light variants in lockstep.
    struct Palette
    {
        wxColour headerBg;
        wxColour headerFg;
        wxColour headerMutedFg;
        wxColour divider;
        wxColour contentBgTop;
        wxColour contentBgBottom;
        wxColour cardBg;
        wxColour cardBgHover;
        wxColour cardBorder;
        wxColour textFg;
        wxColour textMutedFg;
        wxColour accent;
        wxColour statusBg;
        wxColour statusFg;
        wxColour addCardBg;
        wxColour addCardBorder;
        wxColour addIconBg;

        // Surface colour for the secondary panes (dialog bodies, section
        // boxes). Kept distinct from cardBg so a dialog never inherits the
        // "content card" look by accident.
        wxColour surfaceBg;

        // Accent softened for large filled areas; the pure accent is loud
        // when it fills a whole header strip.
        wxColour accentSoft;
    };

    Palette GetPalette(bool darkMode);

    // Slightly stronger variant of a colour, used for hover feedback. Clamped
    // so it never wraps around when the base colour is already near 255.
    wxColour Shift(const wxColour& c, int delta);

    // True when the OS is currently using a dark app theme. The main window
    // resolves this itself for the "follow system" mode; dialogs and native
    // child controls use it to match.
    bool IsSystemDark();

    // Paint a window's non-client frame (title bar, borders) dark or light.
    // wxApp::SetAppearance does not reach the DWM title bar on Windows, so
    // this goes through DwmSetWindowAttribute directly.
    void ApplyDarkFrame(wxWindow* w, bool dark);
}

#endif // UITRAITS_H
