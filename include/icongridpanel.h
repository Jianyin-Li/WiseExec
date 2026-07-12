#ifndef ICONGRIDPANEL_H
#define ICONGRIDPANEL_H

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <vector>
#include <functional>

struct IconGridItem {
    enum Type { TagNull = 0, TagAppItem = 1, TagFuncItem = 2 };

    wxString name;
    wxBitmap icon;
    int tag = TagNull;
    void* data = nullptr;
};

class IconGridPanel : public wxPanel
{
public:
    IconGridPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    ~IconGridPanel();

    void setItems(const std::vector<IconGridItem>& items);
    void setDarkMode(bool dark);
    bool getDarkMode() const { return m_darkMode; }
    const std::vector<IconGridItem>& getItems() const { return m_items; }

    // Callbacks
    std::function<void(int index)> onItemClicked;
    std::function<void(int index, wxPoint pos)> onItemRightClick;

private:
    void OnPaint(wxPaintEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnRightDown(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouseWheel(wxMouseEvent& event);
    void OnEraseBackground(wxEraseEvent& event);

    int hitTest(const wxPoint& pos) const;
    void RefreshHover(const wxPoint& pos);
    void UpdateScrollRange();

    // High-quality rendering via wxGraphicsContext
    void DrawCardGC(wxGraphicsContext* gc, const wxRect& rect,
                    const wxBitmap& icon, const wxString& text,
                    bool hovered, bool selected, bool isAddButton);
    void DrawIconCircular(wxGraphicsContext* gc, const wxBitmap& icon,
                          int cx, int cy, int radius);

    wxDECLARE_EVENT_TABLE();

    std::vector<IconGridItem> m_items;
    std::vector<wxRect> m_itemRects;
    int m_hoveredIndex = -1;
    int m_selectedIndex = -1;
    bool m_darkMode = false;
    int m_scrollY = 0;
    int m_contentHeight = 0;

    // Card layout constants (DPI-aware, set in constructor)
    int m_cardW;
    int m_cardH;
    int m_cardSpacing;
    int m_iconSize;
    int m_cardRadius;
    int m_headerH;
};

#endif // ICONGRIDPANEL_H
