#include "icongenerator.h"
#include <wx/dc.h>
#include <wx/dcmemory.h>
#include <wx/graphics.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <cstring>

// Use simple hash instead of OpenSSL for portability
static unsigned int simpleHash(const wxString& str)
{
    unsigned int hash = 5381;
    for (unsigned int i = 0; i < str.Length(); i++) {
        hash = ((hash << 5) + hash) + static_cast<unsigned int>(str[i]);
    }
    return hash;
}

const std::vector<wxColour>& IconGenerator::getDefaultColors()
{
    static const std::vector<wxColour> colors = {
        wxColour(66, 133, 244),   // Blue
        wxColour(219, 68, 55),    // Red
        wxColour(244, 180, 0),    // Yellow
        wxColour(15, 157, 88),    // Green
        wxColour(171, 71, 188),   // Purple
        wxColour(0, 172, 193),    // Cyan
        wxColour(255, 112, 67),   // Orange
        wxColour(121, 85, 72),    // Brown
        wxColour(158, 158, 158),  // Gray
        wxColour(96, 125, 139)    // Blue Gray
    };
    return colors;
}

wxColour IconGenerator::getColorForName(const wxString& name)
{
    if (name.IsEmpty()) return wxColour(128, 128, 128);

    unsigned int hash = simpleHash(name);
    int colorIndex = hash % getDefaultColors().size();
    return getDefaultColors().at(colorIndex);
}

wxBitmap IconGenerator::generateDefaultIcon(const wxString& name, int size)
{
    if (name.IsEmpty()) {
        return generateIcon(wxT("?"), wxColour(128, 128, 128), *wxWHITE, size);
    }

    wxString firstChar = name.Left(1).Upper();
    wxColour bgColor = getColorForName(name);
    return generateIcon(firstChar, bgColor, *wxWHITE, size);
}

wxBitmap IconGenerator::generateIcon(const wxString& text, const wxColour& backgroundColor,
                                      const wxColour& textColor, int size)
{
    return drawCircularIcon(text, backgroundColor, textColor, size);
}

static wxColour darkenColor(const wxColour& c, int factor)
{
    int r = std::max(0, c.Red() - factor);
    int g = std::max(0, c.Green() - factor);
    int b = std::max(0, c.Blue() - factor);
    return wxColour(r, g, b);
}

wxBitmap IconGenerator::drawCircularIcon(const wxString& text, const wxColour& backgroundColor,
                                          const wxColour& textColor, int size)
{
    wxBitmap bmp(size, size, 32);
    {
        wxMemoryDC dc(bmp);
        dc.SetBackground(wxBrush(wxTransparentColour));
        dc.Clear();

        // Draw circular gradient background
        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc) {
            // Radial gradient
            wxPoint2DDouble center(size / 2.0, size / 2.0);
            wxColour lighter = backgroundColor.ChangeLightness(130);
            wxColour darker = darkenColor(backgroundColor, 40);

            wxGraphicsGradientStops stops(darker, lighter);
            stops.Add(backgroundColor, 0.6f);

            wxGraphicsBrush brush = gc->CreateRadialGradientBrush(
                center.m_x, center.m_y, center.m_x, center.m_y, size / 2.0, stops);
            gc->SetBrush(brush);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawEllipse(2, 2, size - 4, size - 4);

            // Draw text
            int fontSize = static_cast<int>(size * 0.48);
            if (text.Length() > 1) fontSize = static_cast<int>(size * 0.32);

            wxFont font(wxSize(0, fontSize), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
            gc->SetFont(font, textColor);

            wxDouble tw, th;
            gc->GetTextExtent(text, &tw, &th);
            double tx = (size - tw) / 2.0;
            double ty = (size - th) / 2.0;
            gc->DrawText(text, tx, ty);

            delete gc;
        }
    }
    bmp.SetMask(new wxMask(bmp, wxTransparentColour));
    return bmp;
}
