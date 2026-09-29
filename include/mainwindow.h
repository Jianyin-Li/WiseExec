#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <wx/wx.h>
#include <wx/app.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/stattext.h>
#include <memory>
#include <vector>
#include "appitem.h"
#include "breadcrumb.h"
#include "icongridpanel.h"
#include "langselector.h"

class MainWindow : public wxFrame
{
public:
    explicit MainWindow(AppItem* initialItem = nullptr, MainWindow* parentWin = nullptr);
    ~MainWindow();

private:
    // Event handlers
    void OnExit(wxCommandEvent& event);
    void OnAboutQuickStart(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnAddApp(wxCommandEvent& event);
    void OnAddFunc(wxCommandEvent& event);
    void OnOpenConfig(wxCommandEvent& event);
    void OnExtractExeIconToggled(wxCommandEvent& event);
    void OnThemeChanged(wxCommandEvent& event);
    void OnSystemThemeChanged(wxSysColourChangedEvent& event);
    void OnLanguageChanged(wxCommandEvent& event);
    void OnLanguageSelected(int index);
    void OnBackClicked(wxCommandEvent& event);
    void OnItemClicked(int index);
    void OnItemRightClick(int index, wxPoint pos);
    void OnClose(wxCloseEvent& event);

    void RefreshIconList();
    void SaveConfig();
    void LoadConfig();
    void SetupMenuBar();
    void SetupHeaderBar(wxWindow* parent, wxBoxSizer* parentSizer);
    void SetupContextMenu();
    void UpdateHeaderStyle();
    void UpdateBreadcrumb();
    void SyncExtractExeIconMenu();

    // Theme
    void ApplyTheme();
    void ApplyNativeAppearance();
    void SyncThemeMenu();
    static bool IsSystemDark();
    static wxSize DefaultFrameSize();

    void EditItem(int index);
    void DeleteItem(int index);

    // Data
    IconGridPanel* m_gridPanel;
    wxPanel* m_headerBar;
    wxPanel* m_headerDivider;
    BreadcrumbBar* m_breadcrumb;
    wxButton* m_backBtn;
    LangSelector* m_langSelector;
    wxStaticText* m_statusText;
    wxStatusBar* m_statusBar;

    AppItem* m_currentItem;
    AppItem* m_rootItem;
    bool m_darkMode = false;        // resolved dark state actually applied
    wxString m_themeMode = wxT("auto"); // "auto" | "light" | "dark"
    wxString m_savedLanguage;

    // Navigation
    MainWindow* m_rootWindow;
    std::shared_ptr<std::vector<MainWindow*>> m_navStack;
    bool m_navigatingBack = false;

    // Menu
    wxMenu* m_fileMenu;
    wxMenu* m_newMenu;
    wxMenu* m_themeMenu;
    wxMenu* m_aboutMenu;
    wxMenuBar* m_menuBar;

    // Context menu
    wxMenu* m_contextMenu;
    int m_contextIndex = -1;

    enum {
        ID_EXIT = wxID_HIGHEST + 1,
        ID_ABOUT_QS,
        ID_ABOUT_WX,
        ID_ADD_APP,
        ID_ADD_FUNC,
        ID_OPEN_CONFIG,
        ID_EXTRACT_EXE_ICON,
        ID_THEME_AUTO,
        ID_THEME_LIGHT,
        ID_THEME_DARK,
        ID_LANG_CHOICE,
        ID_BACK,
        ID_EDIT,
        ID_DELETE
    };

    wxDECLARE_EVENT_TABLE();
};

#endif // MAINWINDOW_H

