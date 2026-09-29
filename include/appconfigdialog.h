#ifndef APPCONFIGDIALOG_H
#define APPCONFIGDIALOG_H

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include "appitem.h"
#include "iconpreviewpanel.h"
#include "themeddialog.h"

class AppConfigDialog : public ThemedDialog
{
public:
    // `dark` is the main window's resolved theme, so the dialog matches the
    // app even when the OS itself is set to the other mode.
    AppConfigDialog(wxWindow* parent, AppItem* existing = nullptr, bool dark = false);
    ~AppConfigDialog();

    std::shared_ptr<AppItem> getResult() const { return m_result; }

private:
    void OnSelectIcon(wxCommandEvent& event);
    void OnSelectExe(wxCommandEvent& event);
    void OnConfirm(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnNameChanged(wxCommandEvent& event);
    void InitFromItem(AppItem* item);
    void UpdateIconPreview();

    wxTextCtrl* m_nameEdit;
    wxTextCtrl* m_iconEdit;
    wxTextCtrl* m_exeEdit;
    IconPreviewPanel* m_iconPreview;
    std::shared_ptr<AppItem> m_result;
    wxDECLARE_EVENT_TABLE();
};

#endif // APPCONFIGDIALOG_H
