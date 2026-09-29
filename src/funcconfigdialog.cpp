#include "funcconfigdialog.h"
#include "icongenerator.h"
#include "exeiconextractor.h"
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/sizer.h>

// ---------- Event table ----------
wxBEGIN_EVENT_TABLE(FuncConfigDialog, wxDialog)
    EVT_BUTTON(wxID_YES, FuncConfigDialog::OnConfirm)
    EVT_BUTTON(wxID_CANCEL, FuncConfigDialog::OnCancel)
    EVT_BUTTON(wxID_FILE, FuncConfigDialog::OnSelectIcon)
    EVT_BUTTON(wxID_ADD, FuncConfigDialog::OnAddCmd)
    EVT_BUTTON(ID_SELECT_EXE, FuncConfigDialog::OnSelectExe)
    EVT_BUTTON(ID_SELECT_EXE_CMD, FuncConfigDialog::OnSelectExe)
    EVT_BUTTON(wxID_DELETE, FuncConfigDialog::OnDelCmd)
    EVT_TEXT(wxID_ANY, FuncConfigDialog::OnNameChanged)
    EVT_LISTBOX(wxID_ANY, FuncConfigDialog::OnCmdSelected)
    EVT_LISTBOX_DCLICK(wxID_ANY, FuncConfigDialog::OnCmdActivated)
wxEND_EVENT_TABLE()

// ---------- Construction ----------
FuncConfigDialog::FuncConfigDialog(wxWindow* parent, FuncItem* existing, bool dark)
    : ThemedDialog(parent, wxString(_("Function Config")), dark)
    , m_nameEdit(nullptr)
    , m_iconEdit(nullptr)
    , m_exeEdit(nullptr)
    , m_iconPreview(nullptr)
    , m_cmdList(nullptr)
    , m_delCmdBtn(nullptr)
{
    SetSize(FromDIP(520), FromDIP(430));

    wxPanel* panel = GetRootPanel();
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // ---- Top area: form + icon preview side by side ----
    wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);

    // Form (left)
    wxFlexGridSizer* formSizer = new wxFlexGridSizer(2, FromDIP(10), FromDIP(9));
    formSizer->AddGrowableCol(1);

    formSizer->Add(MakeLabel(_("Function Name"), panel), 0, wxALIGN_CENTER_VERTICAL);
    m_nameEdit = new wxTextCtrl(panel, wxID_ANY, wxEmptyString);
    StyleControl(m_nameEdit);
    formSizer->Add(m_nameEdit, 1, wxEXPAND);

    formSizer->Add(MakeLabel(_("Icon Path"), panel), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* iconPathSizer = new wxBoxSizer(wxHORIZONTAL);
    m_iconEdit = MakePathEdit(panel);
    iconPathSizer->Add(m_iconEdit, 1, wxEXPAND);
    iconPathSizer->Add(MakeButton(panel, wxID_FILE, _("Select Icon")),
                       0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(6));
    formSizer->Add(iconPathSizer, 1, wxEXPAND);

    // Executable: optional explicit override. When empty, the icon is taken
    // from this function's own command list.
    formSizer->Add(MakeLabel(_("Exe Path"), panel), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* exePathSizer = new wxBoxSizer(wxHORIZONTAL);
    m_exeEdit = MakePathEdit(panel);
    exePathSizer->Add(m_exeEdit, 1, wxEXPAND);
    exePathSizer->Add(MakeButton(panel, ID_SELECT_EXE, _("Select Exe")),
                      0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(6));
    formSizer->Add(exePathSizer, 1, wxEXPAND);

    topSizer->Add(formSizer, 1, wxEXPAND | wxRIGHT, FromDIP(16));

    // Icon preview (right)
    m_iconPreview = new IconPreviewPanel(panel, FromDIP(64), m_dark);
    topSizer->Add(m_iconPreview, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(topSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(20));

    // ---- Command list ----
    // Drawn as a plain inset list with a caption of its own rather than a
    // wxStaticBox, whose engraved frame cannot follow the palette.
    wxStaticText* cmdCaption = MakeLabel(_("Command List"), panel);

    m_cmdList = new wxListBox(panel, wxID_ANY);
    m_cmdList->SetBackgroundColour(m_dark ? wxColour(0x23, 0x26, 0x2c)
                                          : wxColour(0xff, 0xff, 0xff));
    m_cmdList->SetForegroundColour(m_pal.textFg);
    m_cmdList->SetInitialSize(FromDIP(wxSize(-1, 120)));

    // The list's own buttons: "Enter Command" is the primary action here,
    // "Delete" is only meaningful with a selection so it starts disabled.
    wxBoxSizer* cmdBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    cmdBtnSizer->Add(MakeButton(panel, wxID_ADD, _("Enter Command"), true),
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
    cmdBtnSizer->Add(MakeButton(panel, ID_SELECT_EXE_CMD, _("Select Exe")),
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
    m_delCmdBtn = MakeButton(panel, wxID_DELETE, _("Delete Command"));
    cmdBtnSizer->Add(m_delCmdBtn, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* listSizer = new wxBoxSizer(wxVERTICAL);
    listSizer->Add(cmdCaption, 0, wxBOTTOM, FromDIP(6));
    listSizer->Add(m_cmdList, 1, wxEXPAND | wxBOTTOM, FromDIP(8));
    listSizer->Add(cmdBtnSizer, 0, wxEXPAND);

    mainSizer->Add(listSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(16));

    // ---- Buttons ----
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(MakeButton(panel, wxID_YES, _("Confirm"), true),
                  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
    btnSizer->Add(MakeButton(panel, wxID_CANCEL, _("Cancel")),
                  0, wxALIGN_CENTER_VERTICAL);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(18));

    panel->SetSizer(mainSizer);
    UpdateCmdButtons();
    Layout();

    if (existing) {
        InitFromItem(existing);
    }

    // Start with the name field focused so typing works straight away.
    m_nameEdit->SetFocus();
    m_nameEdit->SelectAll();

    Centre();
}

FuncConfigDialog::~FuncConfigDialog()
{
}

void FuncConfigDialog::InitFromItem(FuncItem* item)
{
    if (!item) return;
    m_nameEdit->SetValue(item->getName());
    m_iconEdit->SetValue(item->getIconPath());
    m_exeEdit->SetValue(item->getExePath());
    for (const auto& cmd : item->getCmds()) {
        m_cmdList->Append(cmd);
    }
    UpdateCmdButtons();
    UpdateIconPreview();
}

void FuncConfigDialog::UpdateCmdButtons()
{
    if (m_delCmdBtn)
        m_delCmdBtn->Enable(m_cmdList && m_cmdList->GetSelection() != wxNOT_FOUND);
}

void FuncConfigDialog::OnCmdSelected(wxCommandEvent&)
{
    UpdateCmdButtons();
}

// Double-clicking a command removes it, matching the list-box convention the
// native control would otherwise apply to the Delete key.
void FuncConfigDialog::OnCmdActivated(wxCommandEvent&)
{
    wxCommandEvent e(wxEVT_BUTTON, wxID_DELETE);
    OnDelCmd(e);
}

void FuncConfigDialog::OnSelectIcon(wxCommandEvent&)
{
    wxFileDialog dlg(this, _("Select Function Icon"), wxGetCwd(),
                     wxEmptyString,
                     wxT("Image Files (*.png;*.jpg;*.ico;*.bmp;*.svg)|*.png;*.jpg;*.ico;*.bmp;*.svg"),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
        m_iconEdit->SetValue(dlg.GetPath());
        UpdateIconPreview();
    }
}

// Browse for an executable. It is added to this function's own command list
// and, when no icon file was picked, also recorded as the explicit icon
// source so the preview matches what will be shown.
void FuncConfigDialog::OnSelectExe(wxCommandEvent&)
{
    wxFileDialog dlg(this, _("Select Executable"), wxGetCwd(),
                     wxEmptyString,
#ifdef __WXMSW__
                     wxT("Executables (*.exe;*.bat;*.cmd;*.com)|*.exe;*.bat;*.cmd;*.com|All Files (*.*)|*.*"),
#else
                     wxT("All Files (*.*)|*.*"),
#endif
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK)
        return;

    const wxString path = dlg.GetPath();

    bool found = false;
    for (unsigned int i = 0; i < m_cmdList->GetCount(); i++) {
        if (m_cmdList->GetString(i) == path) {
            found = true;
            break;
        }
    }
    if (!found)
        m_cmdList->Append(path);

    if (m_iconEdit->GetValue().Trim(true).Trim(false).IsEmpty())
        m_exeEdit->SetValue(path);

    UpdateCmdButtons();
    UpdateIconPreview();
}

void FuncConfigDialog::OnNameChanged(wxCommandEvent&)
{
    UpdateIconPreview();
}

void FuncConfigDialog::UpdateIconPreview()
{
    if (!m_iconPreview) return;
    wxString path = m_iconEdit->GetValue().Trim(true).Trim(false);
    wxString name = m_nameEdit->GetValue().Trim(true).Trim(false);
    wxString exe = m_exeEdit->GetValue().Trim(true).Trim(false);

    wxBitmap icon;
    // Explicit icon file first, then this function's own executable.
    if (!path.IsEmpty() && wxFileName::FileExists(path)) {
        wxImage img(path);
        if (img.IsOk()) {
            int sz = std::max(img.GetWidth(), img.GetHeight());
            if (sz > 64) img.Rescale(64, 64, wxIMAGE_QUALITY_HIGH);
            icon = wxBitmap(img, 32);
        }
    }

    if (!icon.IsOk()) {
        std::vector<wxString> candidates;
        if (!exe.IsEmpty())
            candidates.push_back(exe);
        for (unsigned int i = 0; i < m_cmdList->GetCount(); i++)
            candidates.push_back(m_cmdList->GetString(i));
        icon = ExeIconExtractor::ExtractFirst(candidates, 64);
    }

    if (!icon.IsOk() && !name.IsEmpty()) {
        icon = IconGenerator::generateDefaultIcon(name, 64);
    }
    m_iconPreview->SetIcon(icon);
}

void FuncConfigDialog::OnAddCmd(wxCommandEvent&)
{
    wxTextEntryDialog dlg(this, _("Enter the command to execute (e.g. notepad.exe, calc.exe):"),
                          wxString(_("Enter Command")), wxEmptyString, wxOK | wxCANCEL);
    if (dlg.ShowModal() == wxID_OK) {
        wxString cmd = dlg.GetValue().Trim(true).Trim(false);
        if (!cmd.IsEmpty()) {
            bool found = false;
            for (unsigned int i = 0; i < m_cmdList->GetCount(); i++) {
                if (m_cmdList->GetString(i) == cmd) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                m_cmdList->Append(cmd);
            } else {
                wxMessageBox(_("This command already exists"), wxString(_("Notice")), wxOK | wxICON_INFORMATION, this);
            }
        }
    }
    UpdateCmdButtons();
}

void FuncConfigDialog::OnDelCmd(wxCommandEvent&)
{
    int sel = m_cmdList->GetSelection();
    if (sel != wxNOT_FOUND) {
        m_cmdList->Delete(sel);
    }
    UpdateCmdButtons();
}

void FuncConfigDialog::OnConfirm(wxCommandEvent&)
{
    wxString funcName = m_nameEdit->GetValue().Trim(true).Trim(false);
    wxString iconPath = m_iconEdit->GetValue().Trim(true).Trim(false);
    wxString exePath = m_exeEdit->GetValue().Trim(true).Trim(false);

    if (funcName.IsEmpty()) {
        wxMessageBox(_("Function name cannot be empty"), wxString(_("Notice")), wxOK | wxICON_WARNING, this);
        return;
    }

    if (!iconPath.IsEmpty() && !wxFileName::FileExists(iconPath)) {
        wxMessageBox(_("Icon file does not exist"), wxString(_("Notice")), wxOK | wxICON_WARNING, this);
        return;
    }

    if (!exePath.IsEmpty() && !wxFileName::FileExists(exePath)) {
        wxMessageBox(_("Exe file does not exist"), wxString(_("Notice")), wxOK | wxICON_WARNING, this);
        return;
    }

    if (m_cmdList->GetCount() == 0) {
        wxMessageBox(_("Command list cannot be empty"), wxString(_("Notice")), wxOK | wxICON_WARNING, this);
        return;
    }

    std::vector<wxString> cmds;
    for (unsigned int i = 0; i < m_cmdList->GetCount(); i++) {
        cmds.push_back(m_cmdList->GetString(i));
    }

    m_result = std::make_shared<FuncItem>(funcName, iconPath, cmds, exePath);
    EndModal(wxID_OK);
}

void FuncConfigDialog::OnCancel(wxCommandEvent&)
{
    EndModal(wxID_CANCEL);
}
