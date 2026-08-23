#include "icongridpanel.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/dcmemory.h>
#include <wx/rawbmp.h>
#include <cmath>

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

    // Create a 32-bit result, then apply a soft circular alpha mask so the
    // rim fades out cleanly instead of leaving a hard dark halo.
    wxBitmap result(size, size, 32);
    {
        wxMemoryDC dc(result);
        dc.SetBackground(wxBrush(wxTransparentColour));
        dc.Clear();
        dc.DrawBitmap(wxBitmap(cropped, 32), 0, 0, true);
    }

    wxAlphaPixelData data(result);
    if (data) {
        double r = size / 2.0;
        double cx = r, cy = r;
        const double soft = 1.5; // pixels of fade at the rim
        wxAlphaPixelData::Iterator p(data);
        for (int y = 0; y < size; ++y) {
            p.MoveTo(data, 0, y);
            for (int x = 0; x < size; ++x) {
                double dx = x - cx + 0.5, dy = y - cy + 0.5;
                double dist = std::sqrt(dx * dx + dy * dy);
                double edge = (r - dist) / soft; // >1 inside, <0 outside
                if (edge <= 0.0) {
                    p.Alpha() = 0;
                } else if (edge < 1.0) {
                    p.Alpha() = (unsigned char)(edge * 255.0);
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
    EVT_LEFT_UP(IconGridPanel::OnLeftUp)
    EVT_MOUSE_CAPTURE_LOST(IconGridPanel::OnCaptureLost)
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
    , m_pressedIndex(-1)
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
    m_scrollBarW  = FromDIP(6);
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

void IconGridPanel::OnMouseMove(wxMouseEvent& e)
{
    RefreshHover(e.GetPosition());

    // Track pressed state while the mouse is captured
    if (m_pressedIndex >= 0) {
        int p = hitTest(e.GetPosition());
        if (p != m_pressedIndex) {
            m_pressedIndex = p;
            Refresh();
        }
    }
    e.Skip();
}

void IconGridPanel::OnLeftDown(wxMouseEvent& e)
{
    int idx = hitTest(e.GetPosition());
    m_pressedIndex = idx;
    Refresh();
    if (idx >= 0) {
        CaptureMouse();
    }
    e.Skip();
}

void IconGridPanel::OnLeftUp(wxMouseEvent& e)
{
    int idx = hitTest(e.GetPosition());
    if (idx >= 0 && m_pressedIndex == idx && onItemClicked) {
        onItemClicked(idx);
    }
    m_pressedIndex = -1;
    Refresh();
    if (HasCapture()) {
        ReleaseMouse();
    }
    e.Skip();
}

void IconGridPanel::OnCaptureLost(wxMouseCaptureLostEvent&)
{
    m_pressedIndex = -1;
    Refresh();
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

    // -- Background: subtle vertical gradient --
    wxColour bgTop = m_darkMode ? wxColour(0x1b, 0x1e, 0x26) : wxColour(0xf5, 0xf7, 0xfa);
    wxColour bgBottom = m_darkMode ? wxColour(0x14, 0x16, 0x1c) : wxColour(0xe9, 0xed, 0xf2);
    dc.SetBackground(wxBrush(bgBottom));
    dc.Clear();

    if (sz.x <= 0 || sz.y <= 0) return;

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) return;
    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

    wxGraphicsBrush bgBrush = gc->CreateLinearGradientBrush(0, 0, 0, sz.y, bgTop, bgBottom);
    gc->SetBrush(bgBrush);
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRectangle(0, 0, sz.x, sz.y);

    if (m_items.empty()) {
        delete gc;
        return;
    }

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
        bool pressed = ((int)i == m_pressedIndex);
        bool selected = ((int)i == m_selectedIndex);
        bool isAdd = (m_items[i].tag == IconGridItem::TagNull);

        DrawCardGC(gc, cardRect, m_items[i].icon, m_items[i].name,
                   hovered, pressed, selected, isAdd);
    }

    gc->ResetClip();

    // Scrollbar indicator (overlay)
    DrawScrollbar(gc, sz);

    delete gc;
}

// ---------------------------------------------------------------------------
// DrawScrollbar
// ---------------------------------------------------------------------------
void IconGridPanel::DrawScrollbar(wxGraphicsContext* gc, const wxSize& sz)
{
    int maxScroll = std::max(0, m_contentHeight - sz.y);
    if (maxScroll <= 0) return;

    int trackH = sz.y - FromDIP(16);
    if (trackH <= 0) return;

    double thumbH = std::max((double)FromDIP(32),
                             (double)trackH * sz.y / m_contentHeight);
    double posY = (trackH - thumbH) * (double)m_scrollY / maxScroll + FromDIP(8);

    double x = sz.x - m_scrollBarW - FromDIP(4);

    // Track
    wxColour trackCol = m_darkMode ? wxColour(0xff, 0xff, 0xff, 18)
                                   : wxColour(0x00, 0x00, 0x00, 12);
    gc->SetBrush(gc->CreateBrush(wxBrush(trackCol)));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRoundedRectangle(x, FromDIP(8), m_scrollBarW, trackH,
                             m_scrollBarW / 2.0);

    // Thumb
    wxColour thumbCol = m_darkMode ? wxColour(0xff, 0xff, 0xff, 60)
                                   : wxColour(0x00, 0x00, 0x00, 45);
    gc->SetBrush(gc->CreateBrush(wxBrush(thumbCol)));
    gc->DrawRoundedRectangle(x, posY, m_scrollBarW, thumbH,
                             m_scrollBarW / 2.0);
}

// ---------------------------------------------------------------------------
// DrawCardGC
// ---------------------------------------------------------------------------
void IconGridPanel::DrawCardGC(wxGraphicsContext* gc, const wxRect& rect,
                                const wxBitmap& icon, const wxString& text,
                                bool hovered, bool pressed, bool /*selected*/,
                                bool isAddButton)
{
    double r = m_cardRadius;
    double x = rect.x, y = rect.y, w = rect.width, h = rect.height;

    // Pressed feedback: push the card down and shrink the shadow
    if (pressed && !isAddButton) {
        y += 1;
    }

    // -- Theme accent --
    wxColour accent = m_darkMode ? wxColour(0x4d, 0xa3, 0xff) : wxColour(0x1a, 0x73, 0xe8);

    // -- Colors --
    wxColour cardBg, cardBorder, textFg, iconBg;
    if (isAddButton) {
        cardBg     = m_darkMode ? wxColour(0x28, 0x2a, 0x2e) : wxColour(0xe8, 0xea, 0xed);
        cardBorder = m_darkMode ? wxColour(0x38, 0x3a, 0x3e) : wxColour(0xd8, 0xda, 0xde);
        textFg     = m_darkMode ? wxColour(0x8a, 0x8e, 0x94) : wxColour(0x5f, 0x63, 0x68);
        iconBg     = m_darkMode ? wxColour(0x35, 0x37, 0x3c) : wxColour(0xd8, 0xda, 0xde);
        if (hovered) {
            cardBg     = m_darkMode ? wxColour(0x2f, 0x31, 0x35) : wxColour(0xdf, 0xe7, 0xf4);
            cardBorder = accent;
            textFg     = accent;
            iconBg     = m_darkMode ? wxColour(0x3d, 0x40, 0x46) : wxColour(0xc6, 0xd8, 0xef);
        }
    } else {
        cardBg     = m_darkMode ? (hovered ? wxColour(0x2e, 0x30, 0x35) : wxColour(0x24, 0x26, 0x2a))
                                : (hovered ? wxColour(0xfb, 0xfc, 0xfd) : *wxWHITE);
        cardBorder = hovered ? accent
                    : m_darkMode ? wxColour(0x35, 0x37, 0x3c)
                                 : wxColour(0xe8, 0xea, 0xed);
        textFg     = m_darkMode ? wxColour(0xe0, 0xe2, 0xe6) : wxColour(0x3c, 0x40, 0x43);
        iconBg     = m_darkMode ? wxColour(0x35, 0x37, 0x3c) : wxColour(0xf0, 0xf2, 0xf5);
    }

    // -- Shadow --
    {
        double sa = pressed ? 0.04 : (hovered ? 0.16 : 0.07);
        double base = pressed ? 2.0 : (hovered ? 6.0 : 4.0);
        for (int i = 3; i >= 0; --i) {
            double spread = base + i * (pressed ? 1.5 : (hovered ? 3.0 : 2.0));
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
    gc->SetPen(wxPen(cardBorder, hovered ? 1.4 : 0.8));
    gc->SetBrush(wxBrush(cardBg));
    gc->DrawRoundedRectangle(x, y, w, h, r);

    // Hovering: slightly grow the icon for a "lift" feel
    int iconSize = hovered ? m_iconSize + FromDIP(4) : m_iconSize;
    int iconCX = (int)(x + w / 2);
    int iconCY = (int)(y + m_headerH + m_iconSize / 2 + FromDIP(4));

    if (isAddButton) {
        // Circle with "+"
        double circleR = iconSize / 2.0 - 1;
        gc->SetPen(wxPen(cardBorder, hovered ? 1.6 : 1.0));
        gc->SetBrush(wxBrush(iconBg));
        gc->DrawEllipse(iconCX - circleR, iconCY - circleR, circleR * 2, circleR * 2);

        int ps = iconSize / 3;
        gc->SetPen(wxPen(textFg, hovered ? 2.4 : 2.0));
        gc->StrokeLine(iconCX - ps / 2.0, (double)iconCY,
                       iconCX + ps / 2.0, (double)iconCY);
        gc->StrokeLine((double)iconCX, iconCY - ps / 2.0,
                       (double)iconCX, iconCY + ps / 2.0);
    } else if (icon.IsOk()) {
        // Draw circular icon
        wxBitmap circIcon = MakeCircularIcon(icon, iconSize);
        if (circIcon.IsOk()) {
            int ix = iconCX - iconSize / 2;
            int iy = iconCY - iconSize / 2;
            gc->DrawBitmap(circIcon, ix, iy, iconSize, iconSize);
        }

        // Hover ring only — the icon is already circular via the mask, so a
        // permanent border just adds an ugly dark outline.
        if (hovered) {
            double rad = iconSize / 2.0;
            gc->SetPen(wxPen(accent, 1.6));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->DrawEllipse(iconCX - rad, iconCY - rad, rad * 2, rad * 2);
        }
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
