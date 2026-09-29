#ifndef THEMEDDIALOG_H
#define THEMEDDIALOG_H

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "uitraits.h"

// Shared chrome for the config dialogs. The stock dialog uses whatever the
// platform theme happens to be, which on a dark system leaves a white dialog
// in front of a dark app. This base resolves the app's own palette once,
// paints a matching surface, and gives the child controls the same treatment
// so labels and buttons no longer read as foreign objects.
class ThemedDialog : public wxDialog
{
public:
    ThemedDialog(wxWindow* parent, const wxString& title, bool dark)
        : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
        , m_dark(dark)
    {
        m_pal = UiTraits::GetPalette(dark);

        m_root = new wxPanel(this);
        m_root->SetBackgroundColour(m_pal.surfaceBg);
        m_root->SetForegroundColour(m_pal.textFg);

        // The body is ours, but the title bar and frame belong to the OS, so
        // ask for the matching appearance or a dark dialog gets a white
        // title bar around it.
        UiTraits::ApplyDarkFrame(this, dark);

        wxBoxSizer* frame = new wxBoxSizer(wxVERTICAL);
        frame->Add(m_root, 1, wxEXPAND);
        SetSizer(frame);
    }

    // A field label in the dialog's foreground colour.
    wxStaticText* MakeLabel(const wxString& text, wxWindow* parent) const
    {
        wxStaticText* l = new wxStaticText(parent, wxID_ANY, text);
        StyleControl(l);
        return l;
    }

    // A read-only path field that matches the theme instead of the native
    // white input box.
    wxTextCtrl* MakePathEdit(wxWindow* parent) const
    {
        wxTextCtrl* t = new wxTextCtrl(parent, wxID_ANY);
        t->SetEditable(false);
        StyleControl(t);
        return t;
    }

    wxButton* MakeButton(wxWindow* parent, wxWindowID id, const wxString& label,
                         bool primary = false) const
    {
        wxButton* b = new wxButton(parent, id, label);
        StyleButton(b, primary);
        return b;
    }

    wxPanel* GetRootPanel() const { return m_root; }

protected:
    void StyleControl(wxWindow* w) const
    {
        w->SetBackgroundColour(m_pal.surfaceBg);
        w->SetForegroundColour(m_pal.textFg);
        if (auto* t = dynamic_cast<wxTextCtrl*>(w))
            t->SetBackgroundColour(m_dark ? wxColour(0x23, 0x26, 0x2c) : wxColour(0xff, 0xff, 0xff));
    }

    // Primary buttons get the accent fill; everything else stays flat so
    // there is exactly one obvious action per dialog.
    void StyleButton(wxButton* b, bool primary) const
    {
        b->SetFont(UiTraits::UiFont(9, primary ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL));
        if (primary) {
            b->SetBackgroundColour(m_pal.accent);
            b->SetForegroundColour(*wxWHITE);
        } else {
            b->SetBackgroundColour(m_pal.surfaceBg);
            b->SetForegroundColour(m_pal.textFg);
        }
    }

    UiTraits::Palette m_pal;
    bool m_dark;
    wxPanel* m_root = nullptr;
};

#endif // THEMEDDIALOG_H
