#include "breadcrumb.h"
#include "uitraits.h"

#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>

wxBEGIN_EVENT_TABLE(BreadcrumbBar, wxPanel)
    EVT_PAINT(BreadcrumbBar::OnPaint)
wxEND_EVENT_TABLE()

BreadcrumbBar::BreadcrumbBar(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
    , m_fg(*wxBLACK)
    , m_mutedFg(wxColour(0x80, 0x80, 0x80))
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void BreadcrumbBar::SetSegments(const std::vector<wxString>& segments)
{
    m_segments = segments;

    // Size to the widest single segment, and re-flow on any relayout.
    int maxW = 0;
    {
        wxBitmap bmp(1, 1);
        wxMemoryDC dc(bmp);
        dc.SetFont(UiTraits::UiFontBold(11));
        for (const wxString& s : m_segments)
            maxW = wxMax(maxW, dc.GetTextExtent(s).GetWidth());
    }
    SetMinSize(wxSize(maxW + FromDIP(8), FromDIP(24)));
    Refresh();
}

void BreadcrumbBar::SetColors(const wxColour& fg, const wxColour& mutedFg)
{
    m_fg = fg;
    m_mutedFg = mutedFg;
    Refresh();
}

void BreadcrumbBar::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    if (m_segments.empty())
        return;

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc)
        return;
    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

    const wxFont font = UiTraits::UiFontBold(11);
    gc->SetFont(font, m_fg);

    const wxSize sz = GetClientSize();
    const wxString sep = wxT("  \u25B8  ");

    // Measure first: if the full path does not fit, drop leading segments
    // until it does, so the current folder is never the part that gets cut.
    double sepW = 0.0, th = 0.0;
    gc->GetTextExtent(sep, &sepW, &th);

    std::vector<double> widths(m_segments.size());
    for (size_t i = 0; i < m_segments.size(); ++i)
        gc->GetTextExtent(m_segments[i], &widths[i], &th);

    auto pathWidth = [&](size_t from) {
        double w = 0.0;
        for (size_t i = from; i < m_segments.size(); ++i)
            w += widths[i] + (i > from ? sepW : 0.0);
        return w;
    };

    size_t first = 0;
    while (first + 1 < m_segments.size() && pathWidth(first + 1) <= sz.x)
        ++first;

    double x = 0.0;
    for (size_t i = first; i < m_segments.size(); ++i) {
        if (i > first) {
            gc->SetFont(font, m_mutedFg);
            gc->DrawText(sep, x, (sz.y - th) / 2.0);
            x += sepW;
        }
        // The last segment is the current folder and gets full strength.
        gc->SetFont(font, i + 1 == m_segments.size() ? m_fg : m_mutedFg);
        gc->DrawText(m_segments[i], x, (sz.y - th) / 2.0);
        x += widths[i];
    }

    delete gc;
}
