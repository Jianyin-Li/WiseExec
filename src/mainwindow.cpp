#include "mainwindow.h"
#include "appconfigdialog.h"
#include "funcconfigdialog.h"
#include "icongenerator.h"
#include "config.h"

#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/process.h>
#include <wx/mimetype.h>
#include <wx/url.h>
#include <wx/fs_arc.h>
#include <algorithm>
#include <yaml-cpp/yaml.h>

wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
    EVT_MENU(MainWindow::ID_EXIT, MainWindow::OnExit)
    EVT_MENU(MainWindow::ID_ABOUT_QS, MainWindow::OnAboutQuickStart)
    EVT_MENU(MainWindow::ID_ABOUT_WX, MainWindow::OnAbout)
    EVT_MENU(MainWindow::ID_ADD_APP, MainWindow::OnAddApp)
    EVT_MENU(MainWindow::ID_ADD_FUNC, MainWindow::OnAddFunc)
    EVT_MENU(MainWindow::ID_OPEN_CONFIG, MainWindow::OnOpenConfig)
    EVT_MENU(MainWindow::ID_TOGGLE_DARK, MainWindow::OnToggleDarkMode)
    EVT_CHOICE(MainWindow::ID_LANG_CHOICE, MainWindow::OnLanguageChanged)
    EVT_BUTTON(MainWindow::ID_BACK, MainWindow::OnBackClicked)
    EVT_CLOSE(MainWindow::OnClose)
wxEND_EVENT_TABLE()

MainWindow::MainWindow(AppItem* initialItem, MainWindow* parentWin)
    : wxFrame(nullptr, wxID_ANY, wxT("WiseExec"), wxDefaultPosition, wxSize(800, 600))
    , m_gridPanel(nullptr)
    , m_headerBar(nullptr)
    , m_titleLabel(nullptr)
    , m_backBtn(nullptr)
    , m_langChoice(nullptr)
    , m_langLabel(nullptr)
    , m_statusBar(nullptr)
    , m_currentItem(nullptr)
    , m_rootItem(nullptr)
    , m_darkMode(false)
    , m_rootWindow(nullptr)
    , m_navigatingBack(false)
{
    SetIcon(wxIcon(wxT("IDI_ICON1"), wxBITMAP_TYPE_ICO_RESOURCE));

    // Navigation
    if (parentWin) {
        m_rootWindow = parentWin->m_rootWindow ? parentWin->m_rootWindow : parentWin;
        m_navStack = m_rootWindow->m_navStack;
        m_rootItem = parentWin->m_rootItem;
        m_savedLanguage = parentWin->m_savedLanguage;
        m_darkMode = parentWin->m_darkMode;
    } else {
        m_rootWindow = nullptr;
        m_navStack = std::make_shared<std::vector<MainWindow*>>();
        LoadConfig();
    }

    m_currentItem = initialItem ? initialItem : m_rootItem;

    // Create main panel
    wxPanel* mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Header bar
    SetupHeaderBar(mainPanel, mainSizer);

    // Icon grid
    m_gridPanel = new IconGridPanel(mainPanel);
    m_gridPanel->setDarkMode(m_darkMode);
    m_gridPanel->onItemClicked = [this](int index) { OnItemClicked(index); };
    m_gridPanel->onItemRightClick = [this](int idx, wxPoint pos) { OnItemRightClick(idx, pos); };
    mainSizer->Add(m_gridPanel, 1, wxEXPAND);

    mainPanel->SetSizer(mainSizer);

    // Frame-level sizer so mainPanel fills the frame
    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(mainPanel, 1, wxEXPAND);
    SetSizer(frameSizer);

    SetMinSize(wxSize(640, 480));

    // Menu bar
    SetupMenuBar();

    // Status bar
    m_statusBar = CreateStatusBar(1);

    // Context menu
    SetupContextMenu();

    // Update UI
    RefreshIconList();
    UpdateBreadcrumb();

    Centre();
}

MainWindow::~MainWindow()
{
    // Remove from nav stack
    if (m_navStack) {
        m_navStack->erase(
            std::remove(m_navStack->begin(), m_navStack->end(), this),
            m_navStack->end());
    }
    // Only root deletes rootItem
    if (!m_rootWindow && m_rootItem) {
        delete m_rootItem;
    }
}

void MainWindow::SetupMenuBar()
{
    m_menuBar = new wxMenuBar();

    // File menu
    m_fileMenu = new wxMenu();
    m_newMenu = new wxMenu();
    m_newMenu->Append(ID_ADD_APP, _("App"));
    m_newMenu->Append(ID_ADD_FUNC, _("Function"));
    m_fileMenu->Append(wxID_ANY, _("New"), m_newMenu);
    m_fileMenu->Append(ID_OPEN_CONFIG, _("Open config"));
    m_fileMenu->AppendCheckItem(ID_TOGGLE_DARK, _("Dark Mode"));
    m_fileMenu->AppendSeparator();
    m_fileMenu->Append(ID_EXIT, _("Exit"));

    m_menuBar->Append(m_fileMenu, _("Start"));

    // About menu
    m_aboutMenu = new wxMenu();
    m_aboutMenu->Append(ID_ABOUT_QS, _("About WiseExec"));
    m_aboutMenu->Append(ID_ABOUT_WX, _("About wxWidgets"));

    m_menuBar->Append(m_aboutMenu, _("About"));

    SetMenuBar(m_menuBar);

    // Language selector as corner widget (via statusbar area - use a simple approach)
    // We'll add language choice in the header bar instead
}

void MainWindow::SetupHeaderBar(wxWindow* parent, wxBoxSizer* parentSizer)
{
    m_headerBar = new wxPanel(parent);
    m_headerBar->SetMinSize(wxSize(-1, 52));

    wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);

    // Common font for header elements
    wxFont headerFont(wxSize(0, 12), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    // Back button
    m_backBtn = new wxButton(m_headerBar, ID_BACK, wxT("\u2190"),
                             wxDefaultPosition, wxSize(36, 36), wxBORDER_NONE | wxBU_EXACTFIT);
    m_backBtn->SetFont(wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    m_backBtn->SetBackgroundColour(wxTransparentColour);
    headerSizer->Add(m_backBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(8));
    headerSizer->Hide(m_backBtn); // Hidden initially, shown by UpdateBreadcrumb

    // Title
    m_titleLabel = new wxStaticText(m_headerBar, wxID_ANY, wxEmptyString);
    m_titleLabel->SetFont(headerFont);
    headerSizer->Add(m_titleLabel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(8));

    // Language choice
    m_langLabel = new wxStaticText(m_headerBar, wxID_ANY, _("Language:"));
    m_langLabel->SetFont(headerFont);
    m_langChoice = new wxChoice(m_headerBar, ID_LANG_CHOICE);
    m_langChoice->SetFont(headerFont);
    m_langChoice->Append(wxT("English"));
    m_langChoice->Append(wxT("\x4E2D\x6587")); // 中文

    wxString lang = m_savedLanguage.IsEmpty()
        ? wxString(wxLocale::GetSystemLanguage() == wxLANGUAGE_CHINESE_SIMPLIFIED ? wxT("zh_CN") : wxT("en"))
        : m_savedLanguage;
    m_langChoice->SetSelection(lang == wxT("zh_CN") ? 1 : 0);

    headerSizer->Add(m_langLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
    headerSizer->Add(m_langChoice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));

    m_headerBar->SetSizer(headerSizer);
    parentSizer->Add(m_headerBar, 0, wxEXPAND);

    UpdateHeaderStyle();
}

void MainWindow::UpdateBreadcrumb()
{
    // Determine the root window
    MainWindow* root = m_rootWindow ? m_rootWindow : this;

    // Build breadcrumb: Root ▸ NavStack... ▸ Current
    wxString path;
    wxString rootName = root->m_currentItem && !root->m_currentItem->getName().IsEmpty()
        ? root->m_currentItem->getName() : wxString(_("Home"));
    path = rootName;

    if (m_navStack) {
        for (MainWindow* w : *m_navStack) {
            wxString name = w->m_currentItem && !w->m_currentItem->getName().IsEmpty()
                ? w->m_currentItem->getName() : wxString(_("Home"));
            path += wxT(" \u25B8 ") + name;
        }
    }

    // Append current item if not root
    if (this != root) {
        wxString currentName = m_currentItem && !m_currentItem->getName().IsEmpty()
            ? m_currentItem->getName() : wxString(_("Home"));
        path += wxT(" \u25B8 ") + currentName;
    }

    m_titleLabel->SetLabel(path);

    // Back button: visible only when not at root
    wxSizer* headerSizer = m_headerBar->GetSizer();
    if (headerSizer) {
        headerSizer->Show(m_backBtn, this != root);
    }

    // Update window title and status bar
    wxString currentName = m_currentItem && !m_currentItem->getName().IsEmpty()
        ? m_currentItem->getName() : wxString(_("Home"));
    SetTitle(wxString::Format(wxT("WiseExec - %s"), currentName));
    if (m_statusBar) {
        m_statusBar->SetStatusText(wxString::Format(_("Current: %s"), path));
    }
    Layout();
}

void MainWindow::UpdateHeaderStyle()
{
    if (m_darkMode) {
        m_headerBar->SetBackgroundColour(wxColour(0x0d, 0x47, 0xa1));
        m_titleLabel->SetForegroundColour(wxColour(0xe0, 0xe0, 0xe0));
        m_langLabel->SetForegroundColour(wxColour(0xe0, 0xe0, 0xe0));
        m_backBtn->SetForegroundColour(wxColour(0xe0, 0xe0, 0xe0));
    } else {
        m_headerBar->SetBackgroundColour(wxColour(0x1a, 0x73, 0xe8));
        m_titleLabel->SetForegroundColour(*wxWHITE);
        m_langLabel->SetForegroundColour(*wxWHITE);
        m_backBtn->SetForegroundColour(*wxWHITE);
    }
    m_headerBar->Refresh();
}

void MainWindow::SetupContextMenu()
{
    m_contextMenu = new wxMenu();
    m_contextMenu->Append(ID_EDIT, _("Edit"));
    m_contextMenu->Append(ID_DELETE, _("Delete"));

    // Bind context menu events
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        if (m_contextIndex >= 0) EditItem(m_contextIndex);
    }, ID_EDIT);
    Bind(wxEVT_MENU, [this](wxCommandEvent&) {
        if (m_contextIndex >= 0) DeleteItem(m_contextIndex);
    }, ID_DELETE);
}

void MainWindow::RefreshIconList()
{
    if (!m_currentItem || !m_gridPanel) return;

    std::vector<IconGridItem> items;

    for (const auto& app : m_currentItem->getSubApps()) {
        IconGridItem item;
        item.name = app->getName();
        item.icon = app->getIcon(64);
        item.tag = IconGridItem::TagAppItem;
        item.data = app.get();
        items.push_back(item);
    }

    for (const auto& func : m_currentItem->getFuncs()) {
        IconGridItem item;
        item.name = func->getName();
        item.icon = func->getIcon(64);
        item.tag = IconGridItem::TagFuncItem;
        item.data = func.get();
        items.push_back(item);
    }

    // Add button
    {
        wxColour addBg = m_darkMode ? wxColour(0x3a, 0x3a, 0x3a) : wxColour(0xe8, 0xea, 0xed);
        wxColour addFg = m_darkMode ? wxColour(0x9a, 0xa0, 0xa6) : wxColour(0x5f, 0x63, 0x68);
        IconGridItem item;
        item.name = _("+ Add");
        item.icon = IconGenerator::generateIcon(wxT("+"), addBg, addFg, 64);
        item.tag = IconGridItem::TagNull;
        item.data = nullptr;
        items.push_back(item);
    }

    m_gridPanel->setItems(items);
}

void MainWindow::OnItemClicked(int index)
{
    if (index < 0 || index >= static_cast<int>(m_gridPanel->getItems().size())) return;

    const auto& item = m_gridPanel->getItems()[index];

    if (item.tag == IconGridItem::TagNull) {
        // Show add dialog
        wxMessageDialog dlg(this, _("Please select the type to add:"),
                           _("Select Type"),
                           wxYES_NO | wxCANCEL | wxICON_QUESTION);
        dlg.SetYesNoCancelLabels(_("Add App"), _("Add Function"), _("Cancel"));
        int result = dlg.ShowModal();
        if (result == wxID_YES) {
            wxCommandEvent dummy;
            OnAddApp(dummy);
        } else if (result == wxID_NO) {
            wxCommandEvent dummy;
            OnAddFunc(dummy);
        }
        return;
    }

    if (item.tag == IconGridItem::TagAppItem) {
        auto* appItem = static_cast<AppItem*>(item.data);
        if (m_currentItem->getSubApps().end() !=
            std::find_if(m_currentItem->getSubApps().begin(), m_currentItem->getSubApps().end(),
                [appItem](const std::shared_ptr<AppItem>& p) { return p.get() == appItem; })) {
            if (m_navStack && (m_navStack->empty() || m_navStack->back() != this)) {
                m_navStack->push_back(this);
            }
            MainWindow* newWindow = new MainWindow(appItem, this);
            newWindow->Show();
            this->Hide();
        }
    } else if (item.tag == IconGridItem::TagFuncItem) {
        auto* funcItem = static_cast<FuncItem*>(item.data);
        for (const auto& cmd : funcItem->getCmds()) {
            wxExecute(cmd, wxEXEC_ASYNC);
        }
    }
}

void MainWindow::OnItemRightClick(int index, wxPoint pos)
{
    const auto& items = m_gridPanel->getItems();
    if (index < 0 || index >= static_cast<int>(items.size())) return;
    if (items[index].tag == IconGridItem::TagNull) return;

    m_contextIndex = index;
    m_gridPanel->PopupMenu(m_contextMenu, m_gridPanel->ScreenToClient(pos));
}

void MainWindow::EditItem(int index)
{
    const auto& items = m_gridPanel->getItems();
    if (index < 0 || index >= static_cast<int>(items.size())) return;

    if (items[index].tag == IconGridItem::TagAppItem) {
        auto* appItem = static_cast<AppItem*>(items[index].data);
        AppConfigDialog dlg(this, appItem);
        if (dlg.ShowModal() == wxID_OK) {
            auto result = dlg.getResult();
            if (result) {
                m_currentItem->removeSubApp(appItem);
                m_currentItem->addSubApp(result);
                RefreshIconList();
                SaveConfig();
            }
        }
    } else if (items[index].tag == IconGridItem::TagFuncItem) {
        auto* funcItem = static_cast<FuncItem*>(items[index].data);
        FuncConfigDialog dlg(this, funcItem);
        if (dlg.ShowModal() == wxID_OK) {
            auto result = dlg.getResult();
            if (result) {
                m_currentItem->removeFunc(funcItem);
                m_currentItem->addFunc(result);
                RefreshIconList();
                SaveConfig();
            }
        }
    }
    m_contextIndex = -1;
}

void MainWindow::DeleteItem(int index)
{
    const auto& items = m_gridPanel->getItems();
    if (index < 0 || index >= static_cast<int>(items.size())) return;

    wxString itemName = items[index].name;
    wxMessageDialog dlg(this,
        wxString::Format(_("Are you sure you want to delete \"%s\"?"), itemName),
        _("Confirm Delete"), wxYES_NO | wxICON_QUESTION);

    if (dlg.ShowModal() == wxID_YES) {
        if (items[index].tag == IconGridItem::TagAppItem) {
            auto* appItem = static_cast<AppItem*>(items[index].data);
            m_currentItem->removeSubApp(appItem);
        } else if (items[index].tag == IconGridItem::TagFuncItem) {
            auto* funcItem = static_cast<FuncItem*>(items[index].data);
            m_currentItem->removeFunc(funcItem);
        }
        RefreshIconList();
        SaveConfig();
    }
    m_contextIndex = -1;
}

void MainWindow::OnAddApp(wxCommandEvent&)
{
    AppConfigDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        auto result = dlg.getResult();
        if (result) {
            m_currentItem->addSubApp(result);
            RefreshIconList();
            SaveConfig();
        }
    }
}

void MainWindow::OnAddFunc(wxCommandEvent&)
{
    FuncConfigDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        auto result = dlg.getResult();
        if (result) {
            m_currentItem->addFunc(result);
            RefreshIconList();
            SaveConfig();
        }
    }
}

void MainWindow::OnOpenConfig(wxCommandEvent&)
{
    wxString configPath = wxFileName(wxGetCwd(), AppConfig::CONFIG_FILE_PATH_YAML).GetFullPath();
    if (!wxFileName::FileExists(configPath)) {
        configPath = wxFileName(wxGetCwd(), AppConfig::CONFIG_FILE_PATH).GetFullPath();
    }

    if (wxFileName::FileExists(configPath)) {
        wxLaunchDefaultApplication(configPath);
    } else {
        wxMessageBox(wxString::Format(wxT("Config file not found: %s"), configPath),
                     wxT("Open Config"), wxOK | wxICON_WARNING, this);
    }
}

void MainWindow::OnToggleDarkMode(wxCommandEvent& event)
{
    m_darkMode = event.IsChecked();
    m_gridPanel->setDarkMode(m_darkMode);
    UpdateHeaderStyle();
    RefreshIconList();
    SaveConfig();
}

void MainWindow::OnLanguageChanged(wxCommandEvent&)
{
    int sel = m_langChoice->GetSelection();
    wxString locale = (sel == 1) ? wxT("zh_CN") : wxT("en");

    m_savedLanguage = locale;
    SaveConfig();

    // Relaunch the app to apply the new language
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxExecute(exePath, wxEXEC_ASYNC | wxEXEC_NOHIDE);

    // Close all windows
    for (wxWindow* win : wxTopLevelWindows) {
        win->Close(true);
    }
}

void MainWindow::OnBackClicked(wxCommandEvent&)
{
    m_navigatingBack = true;
    // Pop the current window's parent from the stack
    if (m_navStack && !m_navStack->empty()) {
        m_navStack->pop_back();
    }
    // Show previous window or root
    if (m_navStack && !m_navStack->empty()) {
        MainWindow* prev = m_navStack->back();
        prev->RefreshIconList();
        prev->UpdateBreadcrumb();
        prev->Show();
    } else if (m_rootWindow) {
        m_rootWindow->RefreshIconList();
        m_rootWindow->UpdateBreadcrumb();
        m_rootWindow->Show();
    }
    Close();
}

void MainWindow::OnAboutQuickStart(wxCommandEvent&)
{
    wxString msg = wxT("WiseExec v2.0.0\n\n");
    msg += _("A simple app launcher tool");
    msg += wxT("\n\nCopyright (C) 2024");
    wxMessageBox(msg, _("About WiseExec"), wxOK | wxICON_INFORMATION, this);
}

void MainWindow::OnAbout(wxCommandEvent&)
{
    wxMessageBox(wxString::Format(wxT("wxWidgets %s\n"
                                      "Using %s\n"
                                      "Built with %s"),
                   wxVERSION_STRING,
                   wxPlatformInfo::Get().GetOperatingSystemDescription(),
                   wxVERSION_STRING),
                 _("About wxWidgets"), wxOK | wxICON_INFORMATION, this);
}

void MainWindow::OnExit(wxCommandEvent&)
{
    Close(true);
}

void MainWindow::OnClose(wxCloseEvent&)
{
    SaveConfig();

    if (!m_navigatingBack) {
        // Remove this window from nav stack if present
        if (m_navStack) {
            m_navStack->erase(
                std::remove(m_navStack->begin(), m_navStack->end(), this),
                m_navStack->end());
        }
        // Show previous window or root
        if (m_navStack && !m_navStack->empty()) {
            MainWindow* prev = m_navStack->back();
            prev->RefreshIconList();
            prev->UpdateBreadcrumb();
            prev->Show();
        } else if (m_rootWindow) {
            m_rootWindow->RefreshIconList();
            m_rootWindow->UpdateBreadcrumb();
            m_rootWindow->Show();
        }
    }

    Destroy();
}

void MainWindow::LoadConfig()
{
    // Try YAML first
    wxString yamlPath = wxFileName(wxGetCwd(), AppConfig::CONFIG_FILE_PATH_YAML).GetFullPath();
    if (wxFileName::FileExists(yamlPath)) {
        try {
            YAML::Node rootNode = YAML::LoadFile(yamlPath.ToStdString());
            if (rootNode && rootNode.IsMap()) {
                if (rootNode["language"]) {
                    m_savedLanguage = wxString::FromUTF8(rootNode["language"].as<std::string>().c_str());
                }
                if (rootNode["theme"]) {
                    wxString theme = wxString::FromUTF8(rootNode["theme"].as<std::string>().c_str());
                    m_darkMode = (theme == wxT("dark"));
                }
                m_rootItem = new AppItem();
                m_rootItem->fromYaml(rootNode);
                return;
            }
        } catch (const YAML::Exception&) {
        }
    }

    // Fallback: JSON (simplified - just create default)
    m_rootItem = new AppItem(wxT("Home"), wxT(""));
}

void MainWindow::SaveConfig()
{
    if (!m_rootItem) return;

    YAML::Node rootNode = m_rootItem->toYaml();
    if (m_langChoice) {
        rootNode["language"] = (m_langChoice->GetSelection() == 1) ? "zh_CN" : "en";
    }
    rootNode["theme"] = m_darkMode ? "dark" : "light";

    YAML::Emitter emitter;
    emitter.SetIndent(4);
    emitter << rootNode;

    wxString yamlPath = wxFileName(wxGetCwd(), AppConfig::CONFIG_FILE_PATH_YAML).GetFullPath();
    FILE* f = fopen(yamlPath.ToStdString().c_str(), "w");
    if (f) {
        fprintf(f, "%s", emitter.c_str());
        fclose(f);
    }
}
