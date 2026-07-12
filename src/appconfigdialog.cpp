#include "appconfigdialog.h"
#include "icongenerator.h"
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/dcbuffer.h>
#include <wx/rawbmp.h>

wxBEGIN_EVENT_TABLE(AppConfigDialog, wxDialog)
    EVT_BUTTON(wxID_YES, AppConfigDialog::OnConfirm)
    EVT_BUTTON(wxID_CANCEL, AppConfigDialog::OnCancel)
    EVT_BUTTON(wxID_FILE, AppConfigDialog::OnSelectIcon)
    EVT_TEXT(wxID_ANY, AppConfigDialog::OnNameChanged)
wxEND_EVENT_TABLE()

// ---------- Preview panel: draws a circular icon preview ----------
class IconPreviewPanel : public wxPanel
{
public:
    IconPreviewPanel(wxWindow* parent, int size = 64)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(size, size))
        , m_size(size) {}

    void SetIcon(const wxBitmap& bmp) { m_icon = bmp; Refresh(); }

private:
    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetBackgroundColour()));
        dc.Clear();
        if (!m_icon.IsOk()) return;

        // Scale icon to fit
        int pad = 4;
        int drawSize = m_size - pad * 2;
        wxImage img = m_icon.ConvertToImage();
        int iw = img.GetWidth(), ih = img.GetHeight();
        double scale = std::max((double)drawSize / iw, (double)drawSize / ih);
        img.Rescale((int)(iw * scale), (int)(ih * scale), wxIMAGE_QUALITY_HIGH);

        // Create circular alpha mask
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

        // Border
        dc.SetPen(wxPen(wxColour(0xd0, 0xd0, 0xd0), 1));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawCircle(m_size / 2, m_size / 2, drawSize / 2);
    }

    int m_size;
    wxBitmap m_icon;
    wxDECLARE_EVENT_TABLE();
};
wxBEGIN_EVENT_TABLE(IconPreviewPanel, wxPanel)
    EVT_PAINT(IconPreviewPanel::OnPaint)
wxEND_EVENT_TABLE()

// ---------- AppConfigDialog ----------
AppConfigDialog::AppConfigDialog(wxWindow* parent, AppItem* existing)
    : wxDialog(parent, wxID_ANY, wxString(_("App Config")), wxDefaultPosition,
               wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_iconPreview(nullptr)
{
    SetSize(FromDIP(420), FromDIP(260));
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // ---- Form area ----
    wxFlexGridSizer* formSizer = new wxFlexGridSizer(2, FromDIP(10), FromDIP(8));
    formSizer->AddGrowableCol(1);

    // App name
    formSizer->Add(new wxStaticText(panel, wxID_ANY, _("App Name")),
                   0, wxALIGN_CENTER_VERTICAL);
    m_nameEdit = new wxTextCtrl(panel, wxID_ANY, wxEmptyString);
    formSizer->Add(m_nameEdit, 1, wxEXPAND);

    // Icon path
    formSizer->Add(new wxStaticText(panel, wxID_ANY, _("Icon Path")),
                   0, wxALIGN_CENTER_VERTICAL);
    wxBoxSizer* iconPathSizer = new wxBoxSizer(wxHORIZONTAL);
    m_iconEdit = new wxTextCtrl(panel, wxID_ANY, wxEmptyString);
    m_iconEdit->SetEditable(false);
    iconPathSizer->Add(m_iconEdit, 1, wxEXPAND);
    iconPathSizer->Add(new wxButton(panel, wxID_FILE, _("Select Icon")),
                       0, wxLEFT, FromDIP(6));
    formSizer->Add(iconPathSizer, 1, wxEXPAND);

    mainSizer->Add(formSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(20));

    // ---- Icon preview ----
    m_iconPreview = new IconPreviewPanel(panel, FromDIP(64));
    m_iconPreview->SetBackgroundColour(panel->GetBackgroundColour());
    mainSizer->Add(m_iconPreview, 0, wxALIGN_CENTER | wxTOP, FromDIP(12));

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

AppConfigDialog::~AppConfigDialog()
{
}

void AppConfigDialog::InitFromItem(AppItem* item)
{
    if (!item) return;
    m_nameEdit->SetValue(item->getName());
    m_iconEdit->SetValue(item->getIconPath());
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

void AppConfigDialog::OnNameChanged(wxCommandEvent&)
{
    UpdateIconPreview();
}

void AppConfigDialog::UpdateIconPreview()
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
    static_cast<IconPreviewPanel*>(m_iconPreview)->SetIcon(icon);
}

void AppConfigDialog::OnConfirm(wxCommandEvent&)
{
    wxString appName = m_nameEdit->GetValue().Trim(true).Trim(false);
    wxString iconPath = m_iconEdit->GetValue().Trim(true).Trim(false);

    if (appName.IsEmpty()) {
        wxMessageBox(_("App name cannot be empty"), wxString(_("Notice")), wxOK | wxICON_WARNING, this);
        return;
    }

    if (!iconPath.IsEmpty() && !wxFileName::FileExists(iconPath)) {
        wxMessageBox(_("Icon file does not exist"), wxString(_("Notice")), wxOK | wxICON_WARNING, this);
        return;
    }

    m_result = std::make_shared<AppItem>(appName, iconPath);
    EndModal(wxID_OK);
}

void AppConfigDialog::OnCancel(wxCommandEvent&)
{
    EndModal(wxID_CANCEL);
}
