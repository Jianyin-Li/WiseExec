#ifndef APPCONFIGDIALOG_H
#define APPCONFIGDIALOG_H

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include "appitem.h"

class AppConfigDialog : public wxDialog
{
public:
    AppConfigDialog(wxWindow* parent, AppItem* existing = nullptr);
    ~AppConfigDialog();

    std::shared_ptr<AppItem> getResult() const { return m_result; }

private:
    void OnSelectIcon(wxCommandEvent& event);
    void OnConfirm(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnNameChanged(wxCommandEvent& event);
    void InitFromItem(AppItem* item);
    void UpdateIconPreview();

    wxTextCtrl* m_nameEdit;
    wxTextCtrl* m_iconEdit;
    wxPanel* m_iconPreview;
    std::shared_ptr<AppItem> m_result;

    wxDECLARE_EVENT_TABLE();
};

#endif // APPCONFIGDIALOG_H