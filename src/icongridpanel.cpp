#include "icongridpanel.h"
#include "uitraits.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/dcmemory.h>
#include <wx/rawbmp.h>
#include <cmath>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
// Truncate to fit `maxWidth` pixels, measured with the font that will
// actually draw the text. Counting characters (the old behaviour) breaks for
// mixed Chinese/Latin labels and for wide glyphs.
static wxString EllipsizeToWidth(wxGraphicsContext* gc, const wxString& s,
                                double maxWidth)
{
    if (s.IsEmpty())
        return s;

    wxDouble tw, th;
    gc->GetTextExtent(s, &tw, &th);
    if (tw <= maxWidth)
        return s;

    const wxString ell = wxT("\u2026");
    for (int n = s.length() - 1; n > 0; --n) {
        wxString candidate = s.Left(n) + ell;
        gc->GetTextExtent(candidate, &tw, &th);
        if (tw <= maxWidth)
            return candidate;
    }
    return ell;
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

    // Build a 32-bit result and apply a soft circular alpha mask so the rim
    // fades out cleanly instead of leaving a hard dark halo.
    //
    // This works on the wxImage and only converts to a wxBitmap at the end.
    // Blitting through a wxMemoryDC on top of wxTransparentColour discards
    // the alpha of what was drawn there on wxMSW, which renders every icon
    // that carries an alpha channel (anything extracted from an exe) as an
    // empty circle.
    if (!cropped.HasAlpha())
        cropped.InitAlpha();

    const double r = size / 2.0;
    const double cx = r, cy = r;
    const double soft = 1.5; // pixels of fade at the rim

    unsigned char* alpha = cropped.GetAlpha();
    const int astep = cropped.GetWidth();

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double dx = x - cx + 0.5, dy = y - cy + 0.5;
            const double dist = std::sqrt(dx * dx + dy * dy);
            const double edge = (r - dist) / soft; // >1 inside, <0 outside

            unsigned char mask;
            if (edge <= 0.0)
                mask = 0;
            else if (edge < 1.0)
                mask = (unsigned char)(edge * 255.0);
            else
                mask = 255;

            // Combine the icon's own transparency with the circular mask.
            const unsigned char own = alpha[y * astep + x];
            alpha[y * astep + x] = (unsigned char)((own * mask) / 255);
        }
    }

    return wxBitmap(cropped, 32);
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
    SetMinSize(FromDIP(wxSize(520, 380)));

    m_cardW       = FromDIP(112);
    m_cardH       = FromDIP(116);
    m_cardSpacing = FromDIP(10);
    m_iconSize    = FromDIP(52);
    m_cardRadius  = FromDIP(10);
    m_headerH     = FromDIP(12);
    m_scrollBarW  = FromDIP(6);
    m_pad         = FromDIP(24);
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
    const wxSize sz = GetClientSize();
    const int pad = m_pad;
    const int cols = std::max(1, (sz.x - 2 * pad + m_cardSpacing) / (m_cardW + m_cardSpacing));
    const int rows = ((int)m_items.size() + cols - 1) / cols;
    m_contentHeight = pad + rows * (m_cardH + m_cardSpacing) + pad;
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
    const wxSize sz = GetClientSize();
    const int pad = m_pad;
    const int cols = std::max(1, (sz.x - 2 * pad + m_cardSpacing) / (m_cardW + m_cardSpacing));
    const int totalW = cols * m_cardW + (cols - 1) * m_cardSpacing;
    const int offsetX = (sz.x - totalW) / 2;

    for (size_t i = 0; i < m_items.size(); i++) {
        int col = i % cols;
        int row = i / cols;
        int x = offsetX + col * (m_cardW + m_cardSpacing);
        int y = pad + row * (m_cardH + m_cardSpacing) - m_scrollY;

        wxRect cardRect(x, y, m_cardW, m_cardH);
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
    const UiTraits::Palette pal = UiTraits::GetPalette(m_darkMode);
    wxColour bgTop = pal.contentBgTop;
    wxColour bgBottom = pal.contentBgBottom;
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

    const int pad = m_pad;
    const int cols = std::max(1, (sz.x - 2 * pad + m_cardSpacing) / (m_cardW + m_cardSpacing));
    const int totalW = cols * m_cardW + (cols - 1) * m_cardSpacing;
    const int offsetX = (sz.x - totalW) / 2;

    // Visible area clip
    gc->Clip(0, 0, sz.x, sz.y);

    for (size_t i = 0; i < m_items.size(); i++) {
        int col = i % cols;
        int row = i / cols;
        int x = offsetX + col * (m_cardW + m_cardSpacing);
        int y = pad + row * (m_cardH + m_cardSpacing) - m_scrollY;

        if (y + m_cardH < 0 || y > sz.y) continue;

        wxRect cardRect(x, y, m_cardW, m_cardH);
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
    const UiTraits::Palette pal = UiTraits::GetPalette(m_darkMode);

    double r = m_cardRadius;
    double x = rect.x, y = rect.y, w = rect.width, h = rect.height;

    // Pressed feedback: the card settles by a pixel rather than scaling, so
    // the row of cards never appears to reflow while clicking.
    if (pressed)
        y += 1;

    // -- Colours --
    wxColour cardBg, cardBorder, textFg, iconBg;
    if (isAddButton) {
        cardBg     = hovered ? UiTraits::Shift(pal.addCardBg, m_darkMode ? 10 : -4)
                             : pal.addCardBg;
        cardBorder = hovered ? pal.accent : pal.addCardBorder;
        textFg     = hovered ? pal.accent : pal.textMutedFg;
        iconBg     = hovered ? UiTraits::Shift(pal.addIconBg, m_darkMode ? 10 : -4)
                             : pal.addIconBg;
    } else {
        cardBg     = hovered ? pal.cardBgHover : pal.cardBg;
        cardBorder = hovered ? UiTraits::Shift(pal.cardBorder, m_darkMode ? 12 : -6)
                             : pal.cardBorder;
        textFg     = pal.textFg;
        iconBg     = m_darkMode ? wxColour(0x26, 0x29, 0x2f) : wxColour(0xf2, 0xf4, 0xf7);
    }

    // -- Shadow --
    // A single soft shadow on the hovered card only. At rest the card is
    // defined by its border alone: stacking several spread rectangles made
    // every tile look heavier than the content it holds.
    if (hovered && !isAddButton) {
        const double blur = pressed ? 2.0 : 5.0;
        for (int i = 3; i >= 0; --i) {
            const double t = static_cast<double>(i) / 3.0;
            const double spread = blur * t;
            const unsigned char a =
                static_cast<unsigned char>((pressed ? 10.0 : 26.0) * (1.0 - t * 0.75));
            gc->SetBrush(gc->CreateBrush(wxBrush(wxColour(0, 0, 0, a))));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRoundedRectangle(x - spread, y + blur * 0.35,
                                     w + spread * 2, h + spread * 2, r + spread);
        }
    }

    // -- Card body --
    gc->SetBrush(wxBrush(cardBg));
    // The "+ add" tile is an action, not content, so it keeps the dashed
    // outline that reads as a placeholder.
    if (isAddButton && !hovered)
        gc->SetPen(wxPen(cardBorder, 1.2, wxPENSTYLE_SHORT_DASH));
    else
        gc->SetPen(wxPen(cardBorder, hovered ? 1.0 : 0.8));
    gc->DrawRoundedRectangle(x, y, w, h, r);

    // Hovering lifts the icon slightly for a subtle "picked up" feel.
    int iconSize = hovered ? m_iconSize + FromDIP(2) : m_iconSize;
    const int iconCX = static_cast<int>(x + w / 2);
    const int iconCY = static_cast<int>(y + m_headerH + m_iconSize / 2 + FromDIP(2));

    if (isAddButton) {
        // Circle with "+"
        const double circleR = iconSize / 2.0;
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(iconBg));
        gc->DrawEllipse(iconCX - circleR, iconCY - circleR, circleR * 2, circleR * 2);

        const int ps = iconSize / 3;
        gc->SetPen(wxPen(textFg, hovered ? 2.2 : 1.8));
        gc->StrokeLine(iconCX - ps / 2.0, static_cast<double>(iconCY),
                       iconCX + ps / 2.0, static_cast<double>(iconCY));
        gc->StrokeLine(static_cast<double>(iconCX), iconCY - ps / 2.0,
                       static_cast<double>(iconCX), iconCY + ps / 2.0);
    } else if (icon.IsOk()) {
        wxBitmap circIcon = MakeCircularIcon(icon, iconSize);
        if (circIcon.IsOk()) {
            const int ix = iconCX - iconSize / 2;
            const int iy = iconCY - iconSize / 2;
            gc->DrawBitmap(circIcon, ix, iy, iconSize, iconSize);
        }
    }

    // -- Label --
    if (!text.IsEmpty()) {
        gc->SetFont(UiTraits::UiFont(9), textFg);

        // Sit the label a fixed distance under the icon, measured from the
        // font itself so mixed CJK/Latin text keeps a stable baseline.
        const int textY = iconCY + m_iconSize / 2 + FromDIP(8);

        const double maxTextW = w - FromDIP(12);
        const wxString display = EllipsizeToWidth(gc, text, maxTextW);
        wxDouble tw, th;
        gc->GetTextExtent(display, &tw, &th);

        const double tx = x + (w - tw) / 2.0;

        gc->Clip(x + 2, y, w - 4, h);
        gc->DrawText(display, tx, textY);
        gc->ResetClip();
    }
}
