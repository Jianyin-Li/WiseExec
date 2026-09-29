#include "langselector.h"
#include "uitraits.h"

#include <wx/dcbuffer.h>
#include <wx/display.h>
#include <wx/settings.h>
#include <wx/wx.h>

wxBEGIN_EVENT_TABLE(LangSelector, wxPanel)
    EVT_PAINT(LangSelector::OnPaint)
    EVT_LEFT_DOWN(LangSelector::OnLeftDown)
    EVT_LEAVE_WINDOW(LangSelector::OnLeaveWindow)
    EVT_KEY_DOWN(LangSelector::OnKeyDown)
wxEND_EVENT_TABLE()

LangSelector::LangSelector(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_rowH = FromDIP(28);
    m_text = *wxWHITE;
    m_bg = wxColour(0x1a, 0x6f, 0xe0);
    m_accent = wxColour(0x4d, 0xa3, 0xff);
    m_fill = wxColour(255, 255, 255, 30);
    m_popupWidth = FromDIP(96);

    // Hover is bound here rather than through the event table because
    // EVT_MOUSE_MOVE needs a window capture to keep firing.
    Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        const int h = HitTestItem(e.GetPosition());
        if (h != m_hovered) {
            m_hovered = h;
            Refresh();
        }
    });
    Bind(wxEVT_SET_FOCUS, [this](wxFocusEvent&) { m_hasFocus = true; Refresh(); });
    Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent&) { m_hasFocus = false; Refresh(); });
}

LangSelector::~LangSelector() = default;

void LangSelector::SetEntries(const std::vector<wxString>& labels)
{
    m_labels = labels;
    if (m_selected >= static_cast<int>(m_labels.size()))
        m_selected = 0;
    m_popupWidth = std::max(FromDIP(88), GetSize().x);
    Refresh();
}

void LangSelector::SetSelection(int index)
{
    if (index < 0 || index >= static_cast<int>(m_labels.size()))
        return;
    if (m_selected == index)
        return;
    m_selected = index;
    Refresh();
}

wxString LangSelector::GetLabel() const
{
    if (m_selected < 0 || m_selected >= static_cast<int>(m_labels.size()))
        return wxEmptyString;
    return m_labels[m_selected];
}

void LangSelector::SetColors(const wxColour& text, const wxColour& bg, const wxColour& accent)
{
    m_text = text;
    m_bg = bg;
    m_accent = accent;

    // A fixed translucent white reads as a bright chip against a dark header
    // and is invisible on a light one, so derive the fill from the background
    // instead: a light surface gets a faint dark fill, a dark one a faint
    // light fill.
    const bool bgIsLight = bg.Red() > 128;
    m_fill = bgIsLight ? wxColour(0, 0, 0, 16) : wxColour(255, 255, 255, 30);
    Refresh();
}

int LangSelector::HitTestItem(const wxPoint& clientPos) const
{
    if (clientPos.y < 0 || clientPos.y > GetSize().y)
        return -1;
    return clientPos.x >= 0 && clientPos.x <= GetSize().x ? m_selected : -1;
}

void LangSelector::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(m_bg));
    dc.Clear();

    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc)
        return;
    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

    const wxSize sz = GetSize();
    const double radius = FromDIP(6);

    // Translucent pill so the control reads as a control, not as header text.
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->SetBrush(m_fill);
    gc->DrawRoundedRectangle(0.5, 0.5, sz.x - 1.0, sz.y - 1.0, radius);

    if (m_hasFocus || m_hovered >= 0) {
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->SetPen(wxPen(m_accent, 1.2));
        gc->DrawRoundedRectangle(0.5, 0.5, sz.x - 1.0, sz.y - 1.0, radius);
    }

    const wxString label = GetLabel();
    if (!label.IsEmpty()) {
        gc->SetFont(UiTraits::UiFont(9), m_text);
        wxDouble tw, th;
        gc->GetTextExtent(label, &tw, &th);
        gc->DrawText(label, FromDIP(9), (sz.y - th) / 2.0);

        // Chevron
        const double cx = sz.x - FromDIP(11);
        const double cy = sz.y / 2.0;
        const double w = FromDIP(4.5);
        gc->SetPen(wxPen(m_text, 1.6));
        gc->StrokeLine(cx - w, cy - w * 0.6, cx, cy + w * 0.6);
        gc->StrokeLine(cx, cy + w * 0.6, cx + w, cy - w * 0.6);
    }

    delete gc;
}

void LangSelector::UpdatePopupGeometry()
{
    // The drop-down is drawn into a top-level popup window so it can extend
    // past the header, and is positioned in screen coordinates.
    m_popupWidth = std::max(GetSize().x, FromDIP(96));
}

void LangSelector::OnLeftDown(wxMouseEvent&)
{
    SetFocus();
    if (m_popupOpen) {
        Dismiss();
        return;
    }
    if (m_labels.empty())
        return;
    Popup();
}

void LangSelector::Popup()
{
    if (m_labels.empty())
        return;
    m_popupOpen = true;
    m_popupItem = m_selected;

    const wxPoint screen = ClientToScreen(wxPoint(0, GetSize().y));
    const int height = m_rowH * static_cast<int>(m_labels.size()) + FromDIP(8);

    // Keep the list on screen when the header sits near the bottom edge.
    const wxRect display = wxDisplay(wxDisplay::GetFromPoint(screen)).GetClientArea();
    int y = screen.y;
    if (y + height > display.GetBottom()) {
        y = ClientToScreen(wxPoint(0, 0)).y - height;
    }

    m_popup = new wxPopupTransientWindow(this, wxID_ANY);
    wxPanel* popupPanel = new wxPanel(m_popup, wxID_ANY);
    m_popup->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_popup->Create(popupPanel, wxBORDER_NONE);
    m_popup->SetSize(m_popupWidth, height);
    m_popup->SetPosition(wxPoint(screen.x, y));
    m_popup->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
        const wxPoint p = e.GetPosition();
        const int idx = p.y / m_rowH;
        if (idx >= 0 && idx < static_cast<int>(m_labels.size()))
            Choose(idx);
        else
            Dismiss();
    });
    m_popup->Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) { Dismiss(); });
    m_popup->Bind(wxEVT_PAINT, [this](wxPaintEvent& e) {
        PaintPopup(static_cast<wxWindow*>(e.GetEventObject()));
    });
    m_popup->Bind(wxEVT_MOTION, [this](wxMouseEvent& e) {
        const int idx = e.GetPosition().y / m_rowH;
        if (idx != m_popupItem) {
            m_popupItem = idx;
            m_popup->Refresh();
        }
    });
    m_popup->Show();
    Refresh();
}

void LangSelector::PaintPopup(wxWindow* target)
{
    if (!target)
        return;
    wxAutoBufferedPaintDC dc(target);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc)
        return;
    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

    const wxSize sz = target->GetClientSize();
    const double radius = FromDIP(6);

    const wxColour popupBg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const wxColour popupFg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);

    // Popup sits on a real surface, so it needs its own edge to detach from
    // the header behind it.
    const wxColour popupBorder = wxSystemSettings::GetColour(wxSYS_COLOUR_3DHILIGHT);

    gc->SetBrush(wxBrush(popupBg));
    gc->SetPen(wxPen(popupBorder, 1.0));
    gc->DrawRoundedRectangle(0.5, 0.5, sz.x - 1.0, sz.y - 1.0, radius);

    for (size_t i = 0; i < m_labels.size(); ++i) {
        const bool hot = (static_cast<int>(i) == m_popupItem);
        if (hot) {
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(m_accent));
            gc->DrawRoundedRectangle(FromDIP(3), i * m_rowH + FromDIP(2),
                                     sz.x - FromDIP(6), m_rowH - FromDIP(2), FromDIP(4));
        }
        gc->SetFont(UiTraits::UiFont(9), hot ? *wxWHITE : popupFg);
        wxDouble tw, th;
        gc->GetTextExtent(m_labels[i], &tw, &th);
        gc->DrawText(m_labels[i], FromDIP(9), i * m_rowH + (m_rowH - th) / 2.0);
    }

    delete gc;
}

void LangSelector::Choose(int index)
{
    if (index < 0 || index >= static_cast<int>(m_labels.size())) {
        Dismiss();
        return;
    }
    const bool changed = (index != m_selected);
    m_selected = index;
    Dismiss();
    if (changed && m_onChanged)
        m_onChanged(index);
}

void LangSelector::Dismiss()
{
    if (!m_popupOpen)
        return;
    m_popupOpen = false;
    m_popupItem = -1;
    if (m_popup) {
        m_popup->Destroy();
        m_popup = nullptr;
    }
    Refresh();
}

void LangSelector::OnMouseMove(wxMouseEvent& event)
{
    const int h = HitTestItem(event.GetPosition());
    if (h != m_hovered) {
        m_hovered = h;
        Refresh();
    }
    event.Skip();
}

void LangSelector::OnLeaveWindow(wxMouseEvent& event)
{
    if (m_hovered != -1) {
        m_hovered = -1;
        Refresh();
    }
    event.Skip();
}

void LangSelector::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
        case WXK_UP:
        case WXK_LEFT:
            Choose(m_selected - 1);
            return;
        case WXK_DOWN:
        case WXK_RIGHT:
            Choose(m_selected + 1);
            return;
        case WXK_ESCAPE:
            Dismiss();
            return;
        case WXK_RETURN:
        case WXK_SPACE:
            if (!m_popupOpen)
                Popup();
            return;
        default:
            event.Skip();
    }
}
