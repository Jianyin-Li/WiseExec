#ifndef LANGSELECTOR_H
#define LANGSELECTOR_H

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/graphics.h>
#include <wx/popupwin.h>
#include <wx/vector.h>

#include <functional>
#include <vector>

// A compact, self-drawn replacement for the native wxChoice. The stock
// combo box keeps the platform chrome, which does not follow the header
// background and looks out of place next to the rest of the window.
class LangSelector : public wxPanel
{
public:
    explicit LangSelector(wxWindow* parent, wxWindowID id = wxID_ANY);
    ~LangSelector() override;

    // Entries are (display text, payload). "English" -> "en", "中文" -> "zh_CN".
    void SetEntries(const std::vector<wxString>& labels);
    void SetSelection(int index);
    int GetSelection() const { return m_selected; }
    wxString GetLabel() const;

    void SetColors(const wxColour& text, const wxColour& bg, const wxColour& accent);

    void onChanged(std::function<void(int)> cb) { m_onChanged = std::move(cb); }
private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event) { event.Skip(); Refresh(); }
    void OnLeftDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnLeaveWindow(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void Popup();
    void Dismiss();
    void Choose(int index);
    void PaintPopup(wxWindow* target);
    void UpdatePopupGeometry();

    int HitTestItem(const wxPoint& clientPos) const;

    std::vector<wxString> m_labels;
    int m_selected = 0;
    int m_hovered = -1;   // index under the cursor in the closed control
    int m_popupItem = -1; // index under the cursor in the open list

    bool m_popupOpen = false;
    bool m_hasFocus = false;

    wxColour m_text;
    wxColour m_bg;
    wxColour m_accent;
    wxColour m_fill;  // pill background, derived from m_bg by SetColors

    int m_rowH = 30;
    int m_popupWidth = 0;
    wxPopupTransientWindow* m_popup = nullptr;
    std::function<void(int)> m_onChanged;

    wxDECLARE_EVENT_TABLE();
};

#endif // LANGSELECTOR_H
