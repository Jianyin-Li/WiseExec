#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/panel.h>
#include <wx/menu.h>
#include <wx/statusbr.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <memory>
#include <vector>
#include "appitem.h"
#include "icongridpanel.h"

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
    void OnToggleDarkMode(wxCommandEvent& event);
    void OnLanguageChanged(wxCommandEvent& event);
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

    void EditItem(int index);
    void DeleteItem(int index);

    // Data
    IconGridPanel* m_gridPanel;
    wxPanel* m_headerBar;
    wxStaticText* m_titleLabel;
    wxButton* m_backBtn;
    wxChoice* m_langChoice;
    wxStaticText* m_langLabel;
    wxStatusBar* m_statusBar;

    AppItem* m_currentItem;
    AppItem* m_rootItem;
    bool m_darkMode = false;
    wxString m_savedLanguage;

    // Navigation
    MainWindow* m_rootWindow;
    std::shared_ptr<std::vector<MainWindow*>> m_navStack;
    bool m_navigatingBack = false;

    // Menu
    wxMenu* m_fileMenu;
    wxMenu* m_newMenu;
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
        ID_TOGGLE_DARK,
        ID_LANG_CHOICE,
        ID_BACK,
        ID_EDIT,
        ID_DELETE
    };

    wxDECLARE_EVENT_TABLE();
};

#endif // MAINWINDOW_H

