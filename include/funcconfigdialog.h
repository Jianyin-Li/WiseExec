#ifndef FUNCCONFIGDIALOG_H
#define FUNCCONFIGDIALOG_H

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include "funcitem.h"
#include "iconpreviewpanel.h"
#include "themeddialog.h"

class FuncConfigDialog : public ThemedDialog
{
public:
    // `dark` is the main window's resolved theme, so the dialog matches the
    // app even when the OS itself is set to the other mode.
    FuncConfigDialog(wxWindow* parent, FuncItem* existing = nullptr, bool dark = false);
    ~FuncConfigDialog();

    std::shared_ptr<FuncItem> getResult() const { return m_result; }

private:
    void OnSelectIcon(wxCommandEvent& event);
    void OnAddCmd(wxCommandEvent& event);
    void OnSelectExe(wxCommandEvent& event);
    void OnDelCmd(wxCommandEvent& event);
    void OnConfirm(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnNameChanged(wxCommandEvent& event);
    void OnCmdSelected(wxCommandEvent& event);
    void OnCmdActivated(wxCommandEvent& event);
    void InitFromItem(FuncItem* item);
    void UpdateIconPreview();
    void UpdateCmdButtons();

    wxTextCtrl* m_nameEdit;
    wxTextCtrl* m_iconEdit;
    wxTextCtrl* m_exeEdit;
    IconPreviewPanel* m_iconPreview;
    wxListBox* m_cmdList;
    wxButton* m_delCmdBtn;
    std::shared_ptr<FuncItem> m_result;

    // Dedicated ids for the two "Select Exe" buttons. They used to share
    // wxID_APPLY with the form's own button, so a single handler caught both
    // and the list button could never add a command on its own.
    enum {
        ID_SELECT_EXE = wxID_HIGHEST + 501,
        ID_SELECT_EXE_CMD = wxID_HIGHEST + 502
    };

    wxDECLARE_EVENT_TABLE();
};

#endif // FUNCCONFIGDIALOG_H
