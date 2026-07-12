#include "funcconfigdialog.h"
#include "icongenerator.h"
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/sizer.h>
#include <wx/dcbuffer.h>
#include <wx/rawbmp.h>

// ---------- Preview panel (reused from AppConfigDialog) ----------
class FuncIconPreviewPanel : public wxPanel
{
public:
    FuncIconPreviewPanel(wxWindow* parent, int size = 64)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(size, size))
        , m_size(size) {}

    void SetIcon(const wxBitmap& bmp) { m_icon = bmp; Refresh(); }

private:
    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetBackgroundColour()));
        dc.Clear();
        if (!m_icon.IsOk()) return;

        int pad = 4;
        int drawSize = m_size - pad * 2;
        wxImage img = m_icon.ConvertToImage();
        int iw = img.GetWidth(), ih = img.GetHeight();
        double scale = std::max((double)drawSize / iw, (double)drawSize / ih);
        img.Rescale((int)(iw * scale), (int)(ih * scale), wxIMAGE_QUALITY_HIGH);

        wxBitmap bmp(img, 32);
        {
            wxAlphaPixelData data(bmp);
            if (data) {
                double r = drawSize / 2.0;
                wxAlphaPixelData::Iterator p(data);
                for (int y = 0; y < bmp.GetHeight(); ++y) {
                    p.MoveTo(data, 0, y);
                    for (int x = 0; x < bmp.GetWidth(); ++x) {
                        double dx = x - r + 0.5, dy = y - r + 0.5;
                        if (dx * dx + dy * dy > r * r)
                            p.Alpha() = 0;
                        ++p;
                    }
                }
            }
        }

        dc.DrawBitmap(bmp, pad, pad, true);

        dc.SetPen(wxPen(wxColour(0xd0, 0xd0, 0xd0), 1));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawCircle(m_size / 2, m_size / 2, drawSize / 2);
    }

    int m_size;
    wxBitmap m_icon;
    wxDECLARE_EVENT_TABLE();
};
wxBEGIN_EVENT_TABLE(FuncIconPreviewPanel, wxPanel)
    EVT_PAINT(FuncIconPreviewPanel::OnPaint)
wxEND_EVENT_TABLE()

// ---------- Event table ----------
wxBEGIN_EVENT_TABLE(FuncConfigDialog, wxDialog)
    EVT_BUTTON(wxID_YES, FuncConfigDialog::OnConfirm)
    EVT_BUTTON(wxID_CANCEL, FuncConfigDialog::OnCancel)
    EVT_BUTTON(wxID_FILE, FuncConfigDialog::OnSelectIcon)
    EVT_BUTTON(wxID_ADD, FuncConfigDialog::OnAddCmd)
    EVT_BUTTON(wxID_DELETE, FuncConfigDialog::OnDelCmd)
    EVT_TEXT(wxID_ANY, FuncConfigDialog::OnNameChanged)
wxEND_EVENT_TABLE()

// ---------- Construction ----------
FuncConfigDialog::FuncConfigDialog(wxWindow* parent, FuncItem* existing)
    : wxDialog(parent, wxID_ANY, wxString(_("Function Config")), wxDefaultPosition,
               wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_iconPreview(nullptr)
{
    SetSize(FromDIP(500), FromDIP(420));
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // ---- Top area: form + icon preview side by side ----
    wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);

    // Form (left)
    wxFlexGridSizer* formSizer = new wxFlexGridSizer(2, FromDIP(10), FromDIP(8));
    formSizer->AddGrowableCol(1);

    formSizer->Add(new wxStaticText(panel, wxID_ANY, _("Function Name")),
                   0, wxALIGN_CENTER_VERTICAL);
    m_nameEdit = new wxTextCtrl(panel, wxID_ANY, wxEmptyString);
    formSizer->Add(m_nameEdit, 1, wxEXPAND);

    formSizer->Add(new wxStaticText(panel, wxID_ANY, _("Icon Path")),
                   0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* iconPathSizer = new wxBoxSizer(wxHORIZONTAL);
    m_iconEdit = new wxTextCtrl(panel, wxID_ANY, wxEmptyString);
    m_iconEdit->SetEditable(false);
    iconPathSizer->Add(m_iconEdit, 1, wxEXPAND);
    iconPathSizer->Add(new wxButton(panel, wxID_FILE, _("Select Icon")),
                       0, wxLEFT, FromDIP(6));
    formSizer->Add(iconPathSizer, 1, wxEXPAND);

    topSizer->Add(formSizer, 1, wxEXPAND | wxRIGHT, FromDIP(16));

    // Icon preview (right)
    m_iconPreview = new FuncIconPreviewPanel(panel, FromDIP(64));
    m_iconPreview->SetBackgroundColour(panel->GetBackgroundColour());
    topSizer->Add(m_iconPreview, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(topSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(20));

    // ---- Command List ----
    wxStaticBoxSizer* cmdBox = new wxStaticBoxSizer(wxVERTICAL, panel, _("Command List"));
    m_cmdList = new wxListBox(cmdBox->GetStaticBox(), wxID_ANY);
    cmdBox->Add(m_cmdList, 1, wxEXPAND | wxALL, FromDIP(4));

    wxBoxSizer* cmdBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    cmdBtnSizer->Add(new wxButton(cmdBox->GetStaticBox(), wxID_ADD, _("Enter Command")),
                     0, wxRIGHT, FromDIP(6));
    cmdBtnSizer->Add(new wxButton(cmdBox->GetStaticBox(), wxID_DELETE, _("Delete Command")));
    cmdBox->Add(cmdBtnSizer, 0, wxEXPAND | wxALL, FromDIP(4));

    mainSizer->Add(cmdBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(20));

    // ---- Buttons ----
    wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer();
    btnSizer->Add(new wxButton(panel, wxID_YES, _("Confirm")), 0, wxRIGHT, FromDIP(8));
    btnSizer->Add(new wxButton(panel, wxID_CANCEL, _("Cancel")));
    mainSizer->Add(btnSizer, 0, wxEXPAND | wxALL, FromDIP(12));

    panel->SetSizer(mainSizer);

    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(panel, 1, wxEXPAND);
    SetSizer(frameSizer);

    if (existing) {
        InitFromItem(existing);
    }

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
    for (const auto& cmd : item->getCmds()) {
        m_cmdList->Append(cmd);
    }
    UpdateIconPreview();
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

void FuncConfigDialog::OnNameChanged(wxCommandEvent&)
{
    UpdateIconPreview();
}

void FuncConfigDialog::UpdateIconPreview()
{
    if (!m_iconPreview) return;
    wxString path = m_iconEdit->GetValue().Trim(true).Trim(false);
    wxString name = m_nameEdit->GetValue().Trim(true).Trim(false);

    wxBitmap icon;
    if (!path.IsEmpty() && wxFileName::FileExists(path)) {
        wxImage img(path);
        if (img.IsOk()) {
            int sz = std::max(img.GetWidth(), img.GetHeight());
            if (sz > 64) img.Rescale(64, 64, wxIMAGE_QUALITY_HIGH);
            icon = wxBitmap(img, 32);
        }
    }
    if (!icon.IsOk() && !name.IsEmpty()) {
        icon = IconGenerator::generateDefaultIcon(name, 64);
    }
    static_cast<FuncIconPreviewPanel*>(m_iconPreview)->SetIcon(icon);
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
}

void FuncConfigDialog::OnDelCmd(wxCommandEvent&)
{
    int sel = m_cmdList->GetSelection();
    if (sel != wxNOT_FOUND) {
        m_cmdList->Delete(sel);
    }
}

void FuncConfigDialog::OnConfirm(wxCommandEvent&)
{
    wxString funcName = m_nameEdit->GetValue().Trim(true).Trim(false);
    wxString iconPath = m_iconEdit->GetValue().Trim(true).Trim(false);

    if (funcName.IsEmpty()) {
        wxMessageBox(_("Function name cannot be empty"), wxString(_("Notice")), wxOK | wxICON_WARNING, this);
        return;
    }

    if (!iconPath.IsEmpty() && !wxFileName::FileExists(iconPath)) {
        wxMessageBox(_("Icon file does not exist"), wxString(_("Notice")), wxOK | wxICON_WARNING, this);
        return;
    }

    if (m_cmdList->GetCount() == 0) {
        wxMessageBox(wxT("Command list cannot be empty"), wxT("Notice"), wxOK | wxICON_WARNING, this);
        return;
    }

    std::vector<wxString> cmds;
    for (unsigned int i = 0; i < m_cmdList->GetCount(); i++) {
        cmds.push_back(m_cmdList->GetString(i));
    }

    m_result = std::make_shared<FuncItem>(funcName, iconPath, cmds);
    EndModal(wxID_OK);
}

void FuncConfigDialog::OnCancel(wxCommandEvent&)
{
    EndModal(wxID_CANCEL);
}
