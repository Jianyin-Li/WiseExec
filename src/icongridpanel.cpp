#include "icongridpanel.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/dcmemory.h>
#include <wx/rawbmp.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static wxString Ellipsize(const wxString& s, int maxChars)
{
    if ((int)s.length() <= maxChars) return s;
    if (maxChars <= 2) return s.Left(maxChars);
    return s.Left(maxChars - 1) + wxT("\u2026");
}

// Draw a bitmap clipped to a circle, using a mask approach
static wxBitmap MakeCircularIcon(const wxBitmap& src, int size)
{
    if (!src.IsOk()) return wxNullBitmap;

    // Scale source to size
    wxImage img = src.ConvertToImage();
    int iw = img.GetWidth(), ih = img.GetHeight();
    double scale = std::max((double)size / iw, (double)size / ih);
    int sw = (int)(iw * scale), sh = (int)(ih * scale);
    img.Rescale(sw, sh, wxIMAGE_QUALITY_HIGH);

    // Center-crop to size
    int ox = (sw - size) / 2, oy = (sh - size) / 2;
    wxImage cropped = img.GetSubImage(wxRect(ox, oy, size, size));

    // Create 32-bit result with circular alpha
    wxBitmap result(size, size, 32);
    {
        wxMemoryDC dc(result);
        dc.SetBackground(wxBrush(wxTransparentColour));
        dc.Clear();
        dc.DrawBitmap(wxBitmap(cropped, 32), 0, 0, true);
    }

    // Apply circular alpha mask
    wxAlphaPixelData data(result);
    if (data) {
        double r = size / 2.0;
        double cx = r, cy = r;
        wxAlphaPixelData::Iterator p(data);
        for (int y = 0; y < size; ++y) {
            p.MoveTo(data, 0, y);
            for (int x = 0; x < size; ++x) {
                double dx = x - cx + 0.5, dy = y - cy + 0.5;
                if (dx * dx + dy * dy > r * r) {
                    p.Alpha() = 0;
                }
                ++p;
            }
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Event table
// ---------------------------------------------------------------------------
wxBEGIN_EVENT_TABLE(IconGridPanel, wxPanel)
    EVT_PAINT(IconGridPanel::OnPaint)
    EVT_MOTION(IconGridPanel::OnMouseMove)
    EVT_LEFT_DOWN(IconGridPanel::OnLeftDown)
    EVT_RIGHT_DOWN(IconGridPanel::OnRightDown)
    EVT_SIZE(IconGridPanel::OnSize)
    EVT_MOUSEWHEEL(IconGridPanel::OnMouseWheel)
    EVT_ERASE_BACKGROUND(IconGridPanel::OnEraseBackground)
wxEND_EVENT_TABLE()

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
IconGridPanel::IconGridPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
    , m_hoveredIndex(-1)
    , m_selectedIndex(-1)
    , m_darkMode(false)
    , m_scrollY(0)
    , m_contentHeight(0)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(400, 300));

    m_cardW       = FromDIP(112);
    m_cardH       = FromDIP(132);
    m_cardSpacing = FromDIP(10);
    m_iconSize    = FromDIP(52);
    m_cardRadius  = FromDIP(12);
    m_headerH     = FromDIP(20);
}

IconGridPanel::~IconGridPanel() {}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void IconGridPanel::setItems(const std::vector<IconGridItem>& items)
{
    m_items = items;
    m_hoveredIndex = -1;
    m_scrollY = 0;
    UpdateScrollRange();
    Refresh();
}

void IconGridPanel::setDarkMode(bool dark)
{
    m_darkMode = dark;
    Refresh();
}

void IconGridPanel::UpdateScrollRange()
{
    wxSize sz = GetClientSize();
    int cols = std::max(1, (sz.x + m_cardSpacing) / (m_cardW + m_cardSpacing));
    int rows = ((int)m_items.size() + cols - 1) / cols;
    m_contentHeight = rows * (m_cardH + m_cardSpacing) + m_cardSpacing;
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------
void IconGridPanel::OnSize(wxSizeEvent& event)
{
    UpdateScrollRange();
    Refresh();
    event.Skip();
}

void IconGridPanel::OnEraseBackground(wxEraseEvent&) {}

void IconGridPanel::OnMouseWheel(wxMouseEvent& event)
{
    int delta = event.GetWheelRotation();
    int step = m_cardH / 2;
    int maxScroll = std::max(0, m_contentHeight - GetClientSize().y);
    m_scrollY = wxClip(m_scrollY - delta * step / 120, 0, maxScroll);
    Refresh();
}

int IconGridPanel::hitTest(const wxPoint& pos) const
{
    wxSize sz = GetClientSize();
    int cols = std::max(1, (sz.x + m_cardSpacing) / (m_cardW + m_cardSpacing));
    int totalW = cols * m_cardW + (cols - 1) * m_cardSpacing;
    int offsetX = (sz.x - totalW) / 2;
    int pad = FromDIP(4);

    for (size_t i = 0; i < m_items.size(); i++) {
        int col = i % cols;
        int row = i / cols;
        int x = offsetX + col * (m_cardW + m_cardSpacing) + pad;
        int y = row * (m_cardH + m_cardSpacing) + pad - m_scrollY;

        wxRect cardRect(x, y, m_cardW - 2 * pad, m_cardH - 2 * pad);
        if (cardRect.Contains(pos))
            return static_cast<int>(i);
    }
    return -1;
}

void IconGridPanel::RefreshHover(const wxPoint& pos)
{
    int h = hitTest(pos);
    if (h != m_hoveredIndex) {
        m_hoveredIndex = h;
        Refresh();
    }
}

void IconGridPanel::OnMouseMove(wxMouseEvent& e) { RefreshHover(e.GetPosition()); e.Skip(); }

void IconGridPanel::OnLeftDown(wxMouseEvent& e)
{
    int idx = hitTest(e.GetPosition());
    if (idx >= 0 && onItemClicked) onItemClicked(idx);
    e.Skip();
}

void IconGridPanel::OnRightDown(wxMouseEvent& e)
{
    int idx = hitTest(e.GetPosition());
    if (idx >= 0 && onItemRightClick) onItemRightClick(idx, ClientToScreen(e.GetPosition()));
    e.Skip();
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------
void IconGridPanel::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    wxSize sz = GetClientSize();

    wxColour bg = m_darkMode ? wxColour(0x18, 0x1a, 0x20) : wxColour(0xf0, 0xf2, 0xf5);
    dc.SetBackground(wxBrush(bg));
    dc.Clear();

    if (m_items.empty()) return;

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) return;
    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

    int cols = std::max(1, (sz.x + m_cardSpacing) / (m_cardW + m_cardSpacing));
    int totalW = cols * m_cardW + (cols - 1) * m_cardSpacing;
    int offsetX = (sz.x - totalW) / 2;
    int pad = FromDIP(4);

    // Visible area clip
    gc->Clip(0, 0, sz.x, sz.y);

    for (size_t i = 0; i < m_items.size(); i++) {
        int col = i % cols;
        int row = i / cols;
        int x = offsetX + col * (m_cardW + m_cardSpacing) + pad;
        int y = row * (m_cardH + m_cardSpacing) + pad - m_scrollY;

        if (y + m_cardH < 0 || y > sz.y) continue;

        wxRect cardRect(x, y, m_cardW - 2 * pad, m_cardH - 2 * pad);
        bool hovered = ((int)i == m_hoveredIndex);
        bool selected = ((int)i == m_selectedIndex);
        bool isAdd = (m_items[i].tag == IconGridItem::TagNull);

        DrawCardGC(gc, cardRect, m_items[i].icon, m_items[i].name,
                   hovered, selected, isAdd);
    }

    gc->ResetClip();
    delete gc;
}

// ---------------------------------------------------------------------------
// DrawCardGC
// ---------------------------------------------------------------------------
void IconGridPanel::DrawCardGC(wxGraphicsContext* gc, const wxRect& rect,
                                const wxBitmap& icon, const wxString& text,
                                bool hovered, bool /*selected*/, bool isAddButton)
{
    double r = m_cardRadius;
    double x = rect.x, y = rect.y, w = rect.width, h = rect.height;

    // -- Colors --
    wxColour cardBg, cardBorder, textFg, iconBg;
    if (isAddButton) {
        cardBg     = m_darkMode ? wxColour(0x28, 0x2a, 0x2e) : wxColour(0xe8, 0xea, 0xed);
        cardBorder = m_darkMode ? wxColour(0x38, 0x3a, 0x3e) : wxColour(0xd8, 0xda, 0xde);
        textFg     = m_darkMode ? wxColour(0x8a, 0x8e, 0x94) : wxColour(0x5f, 0x63, 0x68);
        iconBg     = m_darkMode ? wxColour(0x35, 0x37, 0x3c) : wxColour(0xd8, 0xda, 0xde);
    } else {
        cardBg     = m_darkMode ? (hovered ? wxColour(0x2e, 0x30, 0x35) : wxColour(0x24, 0x26, 0x2a))
                                : (hovered ? wxColour(0xf8, 0xf9, 0xfb) : *wxWHITE);
        cardBorder = m_darkMode ? (hovered ? wxColour(0x48, 0x4a, 0x50) : wxColour(0x35, 0x37, 0x3c))
                                : (hovered ? wxColour(0xda, 0xdc, 0xe0) : wxColour(0xe8, 0xea, 0xed));
        textFg     = m_darkMode ? wxColour(0xe0, 0xe2, 0xe6) : wxColour(0x3c, 0x40, 0x43);
        iconBg     = m_darkMode ? wxColour(0x35, 0x37, 0x3c) : wxColour(0xf0, 0xf2, 0xf5);
    }

    // -- Shadow --
    {
        double sa = hovered ? 0.14 : 0.07;
        for (int i = 3; i >= 0; --i) {
            double spread = hovered ? (6.0 + i * 3.0) : (4.0 + i * 2.0);
            double alpha = sa / (i * 0.7 + 1.0);
            unsigned char a = (unsigned char)(alpha * 255);
            wxColour sc(0, 0, 0, a);
            gc->SetBrush(gc->CreateBrush(wxBrush(sc)));
            gc->SetPen(*wxTRANSPARENT_PEN);
            double dx = (i == 0) ? 0 : (i % 2 == 0 ? 1 : -1);
            double dy = spread * 0.5;
            gc->DrawRoundedRectangle(x + dx - i, y + dy - i * 0.5,
                                     w + i * 2, h + i * 2, r + i);
        }
    }

    // -- Card body --
    gc->SetPen(wxPen(cardBorder, hovered ? 1.2 : 0.8));
    gc->SetBrush(wxBrush(cardBg));
    gc->DrawRoundedRectangle(x, y, w, h, r);

    // -- Icon --
    int iconCX = (int)(x + w / 2);
    int iconCY = (int)(y + m_headerH + m_iconSize / 2 + FromDIP(4));

    if (isAddButton) {
        // Circle with "+"
        double circleR = m_iconSize / 2.0 - 1;
        gc->SetPen(wxPen(cardBorder, 1.0));
        gc->SetBrush(wxBrush(iconBg));
        gc->DrawEllipse(iconCX - circleR, iconCY - circleR, circleR * 2, circleR * 2);

        int ps = m_iconSize / 3;
        gc->SetPen(wxPen(textFg, 2.0));
        gc->StrokeLine(iconCX - ps / 2.0, (double)iconCY,
                       iconCX + ps / 2.0, (double)iconCY);
        gc->StrokeLine((double)iconCX, iconCY - ps / 2.0,
                       (double)iconCX, iconCY + ps / 2.0);
    } else if (icon.IsOk()) {
        // Draw circular icon
        wxBitmap circIcon = MakeCircularIcon(icon, m_iconSize);
        if (circIcon.IsOk()) {
            int ix = iconCX - m_iconSize / 2;
            int iy = iconCY - m_iconSize / 2;
            gc->DrawBitmap(circIcon, ix, iy, m_iconSize, m_iconSize);
        }

        // Subtle circular border
        double rad = m_iconSize / 2.0;
        gc->SetPen(wxPen(m_darkMode ? wxColour(0x48, 0x4a, 0x50) : wxColour(0xe0, 0xe2, 0xe6), 0.8));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->DrawEllipse(iconCX - rad, iconCY - rad, rad * 2, rad * 2);
    }

    // -- Text --
    if (!text.IsEmpty() && !isAddButton) {
        int textY = iconCY + m_iconSize / 2 + FromDIP(8);

        gc->SetFont(
            wxFont(FromDIP(10), wxFONTFAMILY_SWISS,
                   wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL),
            textFg);

        wxString display = Ellipsize(text, 14);
        wxDouble tw, th;
        gc->GetTextExtent(display, &tw, &th);

        double tx = x + (w - tw) / 2.0;
        double ty = textY;

        gc->Clip(x + 2, y, w - 4, h);
        gc->DrawText(display, tx, ty);
        gc->ResetClip();
    }
}
