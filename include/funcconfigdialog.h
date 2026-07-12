#ifndef FUNCCONFIGDIALOG_H
#define FUNCCONFIGDIALOG_H

#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include "funcitem.h"

class FuncConfigDialog : public wxDialog
{
public:
    FuncConfigDialog(wxWindow* parent, FuncItem* existing = nullptr);
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
    void InitFromItem(FuncItem* item);
    void UpdateIconPreview();

    wxTextCtrl* m_nameEdit;
    wxTextCtrl* m_iconEdit;
    wxPanel* m_iconPreview;
    wxListBox* m_cmdList;
    std::shared_ptr<FuncItem> m_result;

    wxDECLARE_EVENT_TABLE();
};

#endif // FUNCCONFIGDIALOG_H