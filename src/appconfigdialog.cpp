#include "appconfigdialog.h"
#include "icongenerator.h"
#include "exeiconextractor.h"
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>

wxBEGIN_EVENT_TABLE(AppConfigDialog, wxDialog)
    EVT_BUTTON(wxID_YES, AppConfigDialog::OnConfirm)
    EVT_BUTTON(wxID_CANCEL, AppConfigDialog::OnCancel)
    EVT_BUTTON(wxID_FILE, AppConfigDialog::OnSelectIcon)
    EVT_BUTTON(wxID_APPLY, AppConfigDialog::OnSelectExe)
    EVT_TEXT(wxID_ANY, AppConfigDialog::OnNameChanged)
wxEND_EVENT_TABLE()

// ---------- AppConfigDialog ----------
AppConfigDialog::AppConfigDialog(wxWindow* parent, AppItem* existing, bool dark)
    : ThemedDialog(parent, wxString(_("App Config")), dark)
    , m_nameEdit(nullptr)
    , m_iconEdit(nullptr)
    , m_iconPreview(nullptr)
{
    SetSize(FromDIP(460), FromDIP(236));

    wxPanel* panel = GetRootPanel();
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // ---- Top area: form on the left, icon preview on the right ----
    wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);

    wxFlexGridSizer* formSizer = new wxFlexGridSizer(2, FromDIP(10), FromDIP(9));
    formSizer->AddGrowableCol(1);

    // App name
    formSizer->Add(MakeLabel(_("App Name"), panel), 0, wxALIGN_CENTER_VERTICAL);
    m_nameEdit = new wxTextCtrl(panel, wxID_ANY, wxEmptyString);
    StyleControl(m_nameEdit);
    formSizer->Add(m_nameEdit, 1, wxEXPAND);

    // Icon path
    formSizer->Add(MakeLabel(_("Icon Path"), panel), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* iconPathSizer = new wxBoxSizer(wxHORIZONTAL);
    m_iconEdit = MakePathEdit(panel);
    iconPathSizer->Add(m_iconEdit, 1, wxEXPAND);
    iconPathSizer->Add(MakeButton(panel, wxID_FILE, _("Select Icon")),
                       0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(6));
    formSizer->Add(iconPathSizer, 1, wxEXPAND);

    // Executable
    formSizer->Add(MakeLabel(_("Exe Path"), panel), 0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* exePathSizer = new wxBoxSizer(wxHORIZONTAL);
    m_exeEdit = MakePathEdit(panel);
    exePathSizer->Add(m_exeEdit, 1, wxEXPAND);
    exePathSizer->Add(MakeButton(panel, wxID_APPLY, _("Select Exe")),
                      0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(6));
    formSizer->Add(exePathSizer, 1, wxEXPAND);

    topSizer->Add(formSizer, 1, wxEXPAND | wxRIGHT, FromDIP(16));

    m_iconPreview = new IconPreviewPanel(panel, FromDIP(64), m_dark);
    topSizer->Add(m_iconPreview, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(topSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(20));

    // ---- Buttons ----
    // Confirm is the only filled button, so the way out of the dialog is
    // obvious without weighing it against three equal-weight actions.
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(MakeButton(panel, wxID_YES, _("Confirm"), true),
                  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
    btnSizer->Add(MakeButton(panel, wxID_CANCEL, _("Cancel")),
                  0, wxALIGN_CENTER_VERTICAL);
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(20));

    panel->SetSizer(mainSizer);
    panel->Fit();
    SetMinSize(GetSize());

    if (existing) {
        InitFromItem(existing);
    }

    // Start with the name field focused so typing works straight away.
    m_nameEdit->SetFocus();
    m_nameEdit->SelectAll();

    Centre();
}

AppConfigDialog::~AppConfigDialog()
{
}

void AppConfigDialog::InitFromItem(AppItem* item)
{
    if (!item) return;
    m_nameEdit->SetValue(item->getName());
    m_iconEdit->SetValue(item->getIconPath());
    m_exeEdit->SetValue(item->getExePath());
    UpdateIconPreview();
}

void AppConfigDialog::OnSelectIcon(wxCommandEvent&)
{
    wxFileDialog dlg(this, _("Select App Icon"), wxGetCwd(),
                     wxEmptyString,
                     wxT("Image Files (*.png;*.jpg;*.ico;*.bmp;*.svg)|*.png;*.jpg;*.ico;*.bmp;*.svg"),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
        m_iconEdit->SetValue(dlg.GetPath());
        UpdateIconPreview();
    }
}

void AppConfigDialog::OnSelectExe(wxCommandEvent&)
{
    wxFileDialog dlg(this, _("Select Executable"), wxGetCwd(),
                     wxEmptyString,
#ifdef __WXMSW__
                     wxT("Executables (*.exe;*.bat;*.cmd;*.com)|*.exe;*.bat;*.cmd;*.com|All Files (*.*)|*.*"),
#else
                     wxT("All Files (*.*)|*.*"),
#endif
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
        m_exeEdit->SetValue(dlg.GetPath());
        UpdateIconPreview();
    }
}

void AppConfigDialog::OnNameChanged(wxCommandEvent&)
{
    UpdateIconPreview();
}

void AppConfigDialog::UpdateIconPreview()
{
    if (!m_iconPreview) return;
    wxString path = m_iconEdit->GetValue().Trim(true).Trim(false);
    wxString name = m_nameEdit->GetValue().Trim(true).Trim(false);
    wxString exe = m_exeEdit->GetValue().Trim(true).Trim(false);

    wxBitmap icon;
    // Explicit icon file first, then the executable this app points at.
    if (!path.IsEmpty() && wxFileName::FileExists(path)) {
        wxImage img(path);
        if (img.IsOk()) {
            int sz = std::max(img.GetWidth(), img.GetHeight());
            if (sz > 64) img.Rescale(64, 64, wxIMAGE_QUALITY_HIGH);
            icon = wxBitmap(img, 32);
        }
    }
    if (!icon.IsOk() && !exe.IsEmpty())
        icon = ExeIconExtractor::Extract(exe, 64);
    if (!icon.IsOk() && !name.IsEmpty()) {
        icon = IconGenerator::generateDefaultIcon(name, 64);
    }
    m_iconPreview->SetIcon(icon);
}

void AppConfigDialog::OnConfirm(wxCommandEvent&)
{
    wxString appName = m_nameEdit->GetValue().Trim(true).Trim(false);
    wxString iconPath = m_iconEdit->GetValue().Trim(true).Trim(false);
    wxString exePath = m_exeEdit->GetValue().Trim(true).Trim(false);

    if (appName.IsEmpty()) {
        wxMessageBox(_("App name cannot be empty"), wxString(_("Notice")), wxOK | wxICON_WARNING, this);
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

    m_result = std::make_shared<AppItem>(appName, iconPath, exePath);
    EndModal(wxID_OK);
}

void AppConfigDialog::OnCancel(wxCommandEvent&)
{
    EndModal(wxID_CANCEL);
}
