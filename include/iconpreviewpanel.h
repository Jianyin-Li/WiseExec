#ifndef ICONPREVIEWPANEL_H
#define ICONPREVIEWPANEL_H

#include <wx/bitmap.h>
#include <wx/graphics.h>
#include <wx/panel.h>

#include "uitraits.h"

// Circular icon preview shared by the two config dialogs. It draws through a
// graphics context so the ring, fill and icon all come from the active palette
// instead of a hard-coded grey that looked wrong in dark mode.
class IconPreviewPanel : public wxPanel
{
public:
    IconPreviewPanel(wxWindow* parent, int size, bool dark)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(size, size))
        , m_size(size)
        , m_dark(dark)
    {
        const UiTraits::Palette pal = UiTraits::GetPalette(dark);
        SetBackgroundColour(pal.surfaceBg);
        m_ring = pal.cardBorder;
        m_fill = m_dark ? wxColour(0x26, 0x29, 0x2f) : wxColour(0xf2, 0xf4, 0xf7);
        SetBackgroundStyle(wxBG_STYLE_PAINT);
    }

    void SetIcon(const wxBitmap& bmp)
    {
        m_icon = bmp;
        Refresh();
    }

private:
    void OnPaint(wxPaintEvent&);

    int m_size;
    bool m_dark;
    wxBitmap m_icon;
    wxColour m_ring;
    wxColour m_fill;

    wxDECLARE_EVENT_TABLE();
};

#endif // ICONPREVIEWPANEL_H
