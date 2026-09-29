#include "iconpreviewpanel.h"
#include "uitraits.h"

#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/image.h>

#include <cmath>

wxBEGIN_EVENT_TABLE(IconPreviewPanel, wxPanel)
    EVT_PAINT(IconPreviewPanel::OnPaint)
wxEND_EVENT_TABLE()

// Scale the icon to fit drawSize (aspect preserved) and mask it to a circle.
// The masking happens on the wxImage because that is where the alpha channel
// lives; a wxBitmap only exposes alpha through a mask object, which would
// lose the icon's own per-pixel transparency.
static wxBitmap FittedCircularIcon(const wxBitmap& src, int drawSize)
{
    if (!src.IsOk())
        return wxNullBitmap;

    wxImage img = src.ConvertToImage();
    if (!img.IsOk() || img.GetWidth() <= 0 || img.GetHeight() <= 0)
        return wxNullBitmap;

    const double scale = std::min(static_cast<double>(drawSize) / img.GetWidth(),
                                  static_cast<double>(drawSize) / img.GetHeight());
    const int w = std::max(1, static_cast<int>(img.GetWidth() * scale));
    const int h = std::max(1, static_cast<int>(img.GetHeight() * scale));
    img.Rescale(w, h, wxIMAGE_QUALITY_HIGH);
    if (!img.HasAlpha())
        img.InitAlpha();

    // Mask to an ellipse inscribed in the scaled image, so a non-square icon
    // is cropped by the circle rather than stretched out of round.
    const double r = std::min(w, h) / 2.0;
    const double cx = w / 2.0, cy = h / 2.0;
    unsigned char* alpha = img.GetAlpha();
    const int astep = img.GetWidth();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const double dx = x - cx + 0.5, dy = y - cy + 0.5;
            if (std::sqrt(dx * dx + dy * dy) > r)
                alpha[y * astep + x] = 0;
        }
    }

    return wxBitmap(img, 32);
}

void IconPreviewPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

    const int drawSize = m_size - FromDIP(12);
    const double cx = m_size / 2.0, cy = m_size / 2.0;
    const double r = drawSize / 2.0;

    {
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (!gc)
            return;
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

        // Soft disc behind the icon so a transparent or pale icon still has a
        // defined edge.
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(m_fill));
        gc->DrawEllipse(cx - r, cy - r, r * 2, r * 2);

        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->SetPen(wxPen(m_ring, 1.0));
        gc->DrawEllipse(cx - r, cy - r, r * 2, r * 2);

        delete gc;
    }

    if (!m_icon.IsOk())
        return;

    wxBitmap fitted = FittedCircularIcon(m_icon, drawSize);
    if (!fitted.IsOk())
        return;

    dc.DrawBitmap(fitted, (m_size - fitted.GetWidth()) / 2,
                  (m_size - fitted.GetHeight()) / 2, true);
}
