#include "mainwindow.h"
#include "appconfigdialog.h"
#include "funcconfigdialog.h"
#include "icongenerator.h"
#include "config.h"
#include "uitraits.h"
#include <wx/stdpaths.h>
#include <wx/display.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/textdlg.h>
#include <wx/process.h>
#include <wx/mimetype.h>
#include <wx/url.h>
#include <wx/fs_arc.h>
#ifdef __WXMSW__
#include <windows.h>
#endif
#include <algorithm>
#include <yaml-cpp/yaml.h>

wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
    EVT_MENU(MainWindow::ID_EXIT, MainWindow::OnExit)
    EVT_MENU(MainWindow::ID_ABOUT_QS, MainWindow::OnAboutQuickStart)
    EVT_MENU(MainWindow::ID_ABOUT_WX, MainWindow::OnAbout)
    EVT_MENU(MainWindow::ID_ADD_APP, MainWindow::OnAddApp)
    EVT_MENU(MainWindow::ID_ADD_FUNC, MainWindow::OnAddFunc)
    EVT_MENU(MainWindow::ID_OPEN_CONFIG, MainWindow::OnOpenConfig)
    EVT_MENU(MainWindow::ID_EXTRACT_EXE_ICON, MainWindow::OnExtractExeIconToggled)
    EVT_MENU(MainWindow::ID_THEME_AUTO, MainWindow::OnThemeChanged)
    EVT_MENU(MainWindow::ID_THEME_LIGHT, MainWindow::OnThemeChanged)
    EVT_MENU(MainWindow::ID_THEME_DARK, MainWindow::OnThemeChanged)
    EVT_BUTTON(MainWindow::ID_BACK, MainWindow::OnBackClicked)
    EVT_CLOSE(MainWindow::OnClose)
wxEND_EVENT_TABLE()

// A window size that uses the available screen instead of a fixed 800x600,
// so a small grid is not stranded in the top-left of a large display.
wxSize MainWindow::DefaultFrameSize()
{
    const auto dip = [](int v) { return wxWindowBase::FromDIP(v, nullptr); };

    const wxRect work = wxDisplay(wxDisplay::GetFromPoint(wxPoint(0, 0))).GetClientArea();
    if (work.width <= 0 || work.height <= 0)
        return wxSize(dip(800), dip(600));

    int w = std::min(dip(1000), work.width - dip(60));
    int h = std::min(dip(680), work.height - dip(80));
    return wxSize(std::max(w, dip(560)), std::max(h, dip(420)));
}

MainWindow::MainWindow(AppItem* initialItem, MainWindow* parentWin)
    : wxFrame(nullptr, wxID_ANY, wxT("WiseExec"), wxDefaultPosition, DefaultFrameSize())
    , m_gridPanel(nullptr)
    , m_headerBar(nullptr)
    , m_headerDivider(nullptr)
    , m_breadcrumb(nullptr)
    , m_backBtn(nullptr)
    , m_langSelector(nullptr)
    , m_statusText(nullptr)
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
        m_themeMode = parentWin->m_themeMode;
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

    SetMinSize(FromDIP(wxSize(520, 380)));

    // Ask the OS to paint the window frame (title bar, scrollbars, and the
    // frame around a modal dialog) in the same appearance as the app. Applied
    // here rather than per dialog because the setting is process-wide.
    ApplyNativeAppearance();

    // Menu bar
    SetupMenuBar();

    // Status bar: a plain panel with our own label, because the native
    // status bar draws with system metrics that clash with the header.
    m_statusBar = CreateStatusBar(1);
    m_statusBar->SetMinSize(FromDIP(wxSize(-1, 26)));
    m_statusText = new wxStaticText(m_statusBar, wxID_ANY, wxEmptyString);
    m_statusText->SetFont(UiTraits::UiFont(8));
    {
        wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(m_statusText, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(12));
        m_statusBar->SetSizer(sizer);
    }
    UpdateHeaderStyle(); // re-apply theme now that the status bar exists

    // Context menu
    SetupContextMenu();

    // Keep "follow system" up to date when the OS theme changes
    Bind(wxEVT_SYS_COLOUR_CHANGED, &MainWindow::OnSystemThemeChanged, this);

    // Update UI
    RefreshIconList();
    UpdateBreadcrumb();
    SyncThemeMenu();
    SyncExtractExeIconMenu();

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

    // Theme: follow system, or force light / dark
    m_themeMenu = new wxMenu();
    m_themeMenu->AppendRadioItem(ID_THEME_AUTO, _("Follow System"));
    m_themeMenu->AppendRadioItem(ID_THEME_LIGHT, _("Light"));
    m_themeMenu->AppendRadioItem(ID_THEME_DARK, _("Dark"));
    m_fileMenu->Append(wxID_ANY, _("Theme"), m_themeMenu);

    // Extract app/function icons from their executables when no icon file
    // was configured. Enabled by default.
    m_fileMenu->AppendCheckItem(ID_EXTRACT_EXE_ICON, _("Extract icon from exe"));

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
    m_headerBar->SetMinSize(wxSize(-1, FromDIP(52)));

    wxBoxSizer* headerSizer = new wxBoxSizer(wxHORIZONTAL);

    // Common font for header elements
    const wxFont headerFont = UiTraits::UiFont(9);

    // Back button
    m_backBtn = new wxButton(m_headerBar, ID_BACK, wxT("\u2190"),
                             wxDefaultPosition, FromDIP(wxSize(32, 32)),
                             wxBORDER_NONE | wxBU_EXACTFIT);
    m_backBtn->SetFont(UiTraits::UiFont(12));

    // Hover feedback. The default background is matched to the header colour
    // (set in UpdateHeaderStyle) so the button blends in. We always use a
    // solid colour, never wxTransparentColour — on wxMSW a transparent
    // wxButton paints as a black box.
    m_backBtn->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) {
        const UiTraits::Palette p = UiTraits::GetPalette(m_darkMode);
        m_backBtn->SetBackgroundColour(UiTraits::Shift(p.headerBg, m_darkMode ? 22 : -10));
        m_backBtn->Refresh();
    });
    m_backBtn->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
        m_backBtn->SetBackgroundColour(m_headerBar->GetBackgroundColour());
        m_backBtn->Refresh();
    });

    headerSizer->Add(m_backBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(12));
    headerSizer->Hide(m_backBtn); // Hidden initially, shown by UpdateBreadcrumb

    // Title (breadcrumb) - the current folder is drawn at full strength and
    // its ancestors dimmed, so the endpoint reads immediately.
    m_breadcrumb = new BreadcrumbBar(m_headerBar);
    headerSizer->Add(m_breadcrumb, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(10));

    // Language choice. The label is dropped in favour of a self-describing
    // control, which keeps the header from carrying a second piece of text.
    m_langSelector = new LangSelector(m_headerBar, ID_LANG_CHOICE);
    m_langSelector->SetMinSize(FromDIP(wxSize(92, 28)));
    m_langSelector->SetEntries({ wxT("English"), wxT("\x4E2D\x6587") }); // 中文

    wxString lang = m_savedLanguage.IsEmpty()
        ? wxString(wxLocale::GetSystemLanguage() == wxLANGUAGE_CHINESE_SIMPLIFIED ? wxT("zh_CN") : wxT("en"))
        : m_savedLanguage;
    m_langSelector->SetSelection(lang == wxT("zh_CN") ? 1 : 0);
    m_langSelector->onChanged([this](int index) { OnLanguageSelected(index); });

    headerSizer->Add(m_langSelector, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));

    m_headerBar->SetSizer(headerSizer);
    parentSizer->Add(m_headerBar, 0, wxEXPAND);

    // Thin divider under the header for visual separation
    m_headerDivider = new wxPanel(parent);
    m_headerDivider->SetMinSize(wxSize(-1, FromDIP(1)));
    parentSizer->Add(m_headerDivider, 0, wxEXPAND);

    UpdateHeaderStyle();
}

void MainWindow::UpdateBreadcrumb()
{
    // Determine the root window
    MainWindow* root = m_rootWindow ? m_rootWindow : this;

    // Build breadcrumb as a list of segments so ancestors can be dimmed and
    // the current folder reads as the endpoint.
    std::vector<wxString> segs;
    segs.push_back(root->m_currentItem && !root->m_currentItem->getName().IsEmpty()
                       ? root->m_currentItem->getName() : wxString(_("Home")));

    if (m_navStack) {
        for (MainWindow* w : *m_navStack) {
            if (w == root) continue; // root is already the first segment
            segs.push_back(w->m_currentItem && !w->m_currentItem->getName().IsEmpty()
                               ? w->m_currentItem->getName() : wxString(_("Home")));
        }
    }

    if (this != root) {
        segs.push_back(m_currentItem && !m_currentItem->getName().IsEmpty()
                           ? m_currentItem->getName() : wxString(_("Home")));
    }

    m_breadcrumb->SetSegments(segs);

    // Back button: visible only when not at root
    wxSizer* headerSizer = m_headerBar->GetSizer();
    if (headerSizer) {
        headerSizer->Show(m_backBtn, this != root);
    }

    // Update window title and status bar
    wxString currentName = m_currentItem && !m_currentItem->getName().IsEmpty()
        ? m_currentItem->getName() : wxString(_("Home"));
    SetTitle(wxString::Format(wxT("WiseExec - %s"), currentName));
    if (m_statusText) {
        wxString path;
        for (size_t i = 0; i < segs.size(); ++i)
            path = i ? path + wxT(" \u25B8 ") + segs[i] : segs[i];
        m_statusText->SetLabel(wxString::Format(_("Current: %s"), path));
    }
    Layout();
}

void MainWindow::UpdateHeaderStyle()
{
    const UiTraits::Palette pal = UiTraits::GetPalette(m_darkMode);

    m_headerBar->SetBackgroundColour(pal.headerBg);
    m_breadcrumb->SetBackgroundColour(pal.headerBg);
    m_breadcrumb->SetColors(pal.headerFg, pal.headerMutedFg);
    m_backBtn->SetForegroundColour(pal.headerFg);
    m_headerDivider->SetBackgroundColour(pal.divider);
    if (m_statusBar) {
        m_statusBar->SetBackgroundColour(pal.statusBg);
        m_statusBar->SetForegroundColour(pal.statusFg);
    }
    if (m_statusText) {
        m_statusText->SetForegroundColour(pal.statusFg);
        m_statusText->SetFont(UiTraits::UiFont(9));
        m_statusText->Refresh();
    }
    if (m_langSelector)
        m_langSelector->SetColors(pal.headerFg, pal.headerBg, pal.accent);

    if (m_backBtn) {
        m_backBtn->SetBackgroundColour(m_headerBar->GetBackgroundColour());
    }
    m_headerBar->Refresh();
    if (m_headerDivider) m_headerDivider->Refresh();
    if (m_statusBar) m_statusBar->Refresh();
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

    // Add button. The glyph is drawn by the grid itself, so the bitmap here
    // only has to carry the right colours.
    {
        const UiTraits::Palette pal = UiTraits::GetPalette(m_darkMode);
        IconGridItem item;
        item.name = _("+ Add");
        item.icon = IconGenerator::generateIcon(wxT("+"), pal.addIconBg,
                                                pal.textMutedFg, 64);
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
            // Root window is the base of the breadcrumb and is restored via
            // m_rootWindow; only intermediate windows go on the nav stack so
            // "back" returns to the previous level (not all the way to root).
            MainWindow* root = m_rootWindow ? m_rootWindow : this;
            if (m_navStack && this != root &&
                (m_navStack->empty() || m_navStack->back() != this)) {
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
        AppConfigDialog dlg(this, appItem, m_darkMode);
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
        FuncConfigDialog dlg(this, funcItem, m_darkMode);
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
    AppConfigDialog dlg(this, nullptr, m_darkMode);
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
    FuncConfigDialog dlg(this, nullptr, m_darkMode);
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

void MainWindow::SyncExtractExeIconMenu()
{
    if (m_fileMenu)
        m_fileMenu->Check(ID_EXTRACT_EXE_ICON, AppConfig::extractExeIcon);
}

void MainWindow::OnExtractExeIconToggled(wxCommandEvent& event)
{
    AppConfig::extractExeIcon = event.IsChecked();
    SyncExtractExeIconMenu();
    RefreshIconList();
    SaveConfig();
}

void MainWindow::OnThemeChanged(wxCommandEvent& event)
{
    switch (event.GetId()) {
        case ID_THEME_AUTO:  m_themeMode = wxT("auto");  break;
        case ID_THEME_LIGHT: m_themeMode = wxT("light"); break;
        case ID_THEME_DARK:  m_themeMode = wxT("dark");  break;
        default: return;
    }
    ApplyTheme();
    SaveConfig();
}

// Resolve the effective dark flag from the chosen mode and apply everywhere.
void MainWindow::ApplyTheme()
{
    if (m_themeMode == wxT("auto"))
        m_darkMode = IsSystemDark();
    else
        m_darkMode = (m_themeMode == wxT("dark"));

    ApplyNativeAppearance();

    if (m_gridPanel) {
        m_gridPanel->setDarkMode(m_darkMode);
        RefreshIconList();
    }
    UpdateHeaderStyle();
    SyncThemeMenu();
}

// The window frame itself is drawn by the OS, so it needs its own switch;
// without this a dark app still gets a white title bar.
void MainWindow::ApplyNativeAppearance()
{
    // wxApp::SetAppearance()/wxApp::Appearance were introduced in wxWidgets
    // 3.3. The Linux CI builds against the distribution wxWidgets 3.2
    // (libwxgtk3.2-dev), where the enum and the method do not exist at all,
    // so the call has to be compiled out there. Platforms other than MSW
    // follow the system appearance automatically, which is what the Linux
    // and macOS builds would get from the call anyway.
#if wxCHECK_VERSION(3, 3, 0)
    if (wxApp* app = wxTheApp) {
        app->SetAppearance(m_darkMode ? wxApp::Appearance::Dark
                                       : wxApp::Appearance::Light);
    }
#endif
    UiTraits::ApplyDarkFrame(this, m_darkMode);
}

void MainWindow::SyncThemeMenu()
{
    if (!m_themeMenu) return;
    int id = ID_THEME_AUTO;
    if (m_themeMode == wxT("light"))      id = ID_THEME_LIGHT;
    else if (m_themeMode == wxT("dark"))  id = ID_THEME_DARK;
    m_themeMenu->Check(id, true);
}

bool MainWindow::IsSystemDark()
{
    return UiTraits::IsSystemDark();
}

void MainWindow::OnSystemThemeChanged(wxSysColourChangedEvent&)
{
    if (m_themeMode == wxT("auto"))
        ApplyTheme();
}

void MainWindow::OnLanguageChanged(wxCommandEvent& event)
{
    OnLanguageSelected(m_langSelector ? m_langSelector->GetSelection()
                                      : static_cast<int>(event.GetSelection()));
}

void MainWindow::OnLanguageSelected(int sel)
{
    wxString locale = (sel == 1) ? wxT("zh_CN") : wxT("en");

    if (locale == m_savedLanguage)
        return; // the selector already shows this value; nothing to restart

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
    // The nav stack holds the ancestor chain; its top is this window's
    // parent (the previous level). Show it, then pop it.
    MainWindow* prev = nullptr;
    if (m_navStack && !m_navStack->empty()) {
        prev = m_navStack->back();
        m_navStack->pop_back();
    }
    if (prev) {
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
    // Built from the shared app metadata so the About box cannot drift from
    // the VERSION file / version resource.
    wxString msg;
    msg << AppConfig::APP_DISPLAY_NAME << wxT(" v") << AppConfig::APP_VERSION << wxT("\n\n");
    msg << AppConfig::APP_DESCRIPTION;
    msg << wxT("\n\n") << AppConfig::APP_COPYRIGHT;
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
        // Show the previous level (top of stack) and pop it, keeping the
        // invariant that a displayed window is never on the stack.
        if (m_navStack && !m_navStack->empty()) {
            MainWindow* prev = m_navStack->back();
            m_navStack->pop_back();
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
                    if (theme == wxT("light") || theme == wxT("dark") || theme == wxT("auto"))
                        m_themeMode = theme;
                    else
                        m_themeMode = wxT("auto");
                    m_darkMode = (m_themeMode == wxT("auto"))
                                     ? IsSystemDark()
                                     : (m_themeMode == wxT("dark"));
                }
                // Absent key keeps the default, which is enabled.
                if (rootNode["extractExeIcon"])
                    AppConfig::extractExeIcon = rootNode["extractExeIcon"].as<bool>();
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
    if (m_langSelector) {
        rootNode["language"] = (m_langSelector->GetSelection() == 1) ? "zh_CN" : "en";
    }
    rootNode["theme"] = m_themeMode.ToStdString();
    rootNode["extractExeIcon"] = AppConfig::extractExeIcon;

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
