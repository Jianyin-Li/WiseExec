#ifndef BREADCRUMB_H
#define BREADCRUMB_H

#include <wx/panel.h>
#include <wx/graphics.h>
#include <wx/string.h>

#include <vector>

// A self-drawn breadcrumb strip. wxStaticText can only paint a label in one
// colour, so the current folder could not be highlighted against its dimmed
// ancestors. Drawing the segments directly gives the hierarchy for free and
// keeps the header free of native chrome.
class BreadcrumbBar : public wxPanel
{
public:
    explicit BreadcrumbBar(wxWindow* parent, wxWindowID id = wxID_ANY);

    // Entries are ordered from the root down to the current folder; the last
    // one is always drawn in the full-strength foreground.
    void SetSegments(const std::vector<wxString>& segments);

    void SetColors(const wxColour& fg, const wxColour& mutedFg);

private:
    void OnPaint(wxPaintEvent& event);

    std::vector<wxString> m_segments;
    wxColour m_fg;
    wxColour m_mutedFg;

    wxDECLARE_EVENT_TABLE();
};

#endif // BREADCRUMB_H
