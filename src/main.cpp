#include "mainwindow.h"
#include "config.h"
#include "translations.h"

#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <yaml-cpp/yaml.h>

static wxString LoadLanguageFromConfig()
{
    wxString yamlPath = wxFileName(wxGetCwd(), AppConfig::CONFIG_FILE_PATH_YAML).GetFullPath();
    if (wxFileName::FileExists(yamlPath)) {
        try {
            YAML::Node rootNode = YAML::LoadFile(yamlPath.ToStdString());
            if (rootNode && rootNode.IsMap() && rootNode["language"]) {
                return wxString::FromUTF8(rootNode["language"].as<std::string>().c_str());
            }
        } catch (const YAML::Exception&) {
        }
    }
    return wxEmptyString;
}

// Build the embedded Chinese translation catalog
static EmbeddedTranslationsLoader* CreateTranslationLoader()
{
    auto* loader = new EmbeddedTranslationsLoader();
    using SP = std::pair<wxString, wxString>;
    loader->AddStrings(wxT("zh_CN"), {
        // MainWindow - menus
        SP(wxT("App"),           wxT("应用")),
        SP(wxT("Function"),      wxT("功能")),
        SP(wxT("New"),           wxT("新建")),
        SP(wxT("Open config"),   wxT("打开配置")),
        SP(wxT("Theme"),         wxT("主题")),
        SP(wxT("Follow System"), wxT("跟随系统")),
        SP(wxT("Light"),         wxT("浅色")),
        SP(wxT("Dark"),          wxT("深色")),
        SP(wxT("Exit"),          wxT("退出")),
        SP(wxT("Start"),         wxT("开始")),
        SP(wxT("About WiseExec"), wxT("关于 WiseExec")),
        SP(wxT("About wxWidgets"), wxT("关于 wxWidgets")),
        SP(wxT("About"),         wxT("关于")),
        // MainWindow - header & breadcrumb
        SP(wxT("Language:"),     wxT("语言:")),
        SP(wxT("Home"),          wxT("主页")),
        SP(wxT("Current: %s"),   wxT("当前: %s")),
        // MainWindow - context menu & actions
        SP(wxT("Edit"),          wxT("编辑")),
        SP(wxT("Delete"),        wxT("删除")),
        SP(wxT("+ Add"),         wxT("+ 添加")),
        SP(wxT("Please select the type to add:"), wxT("请选择要添加的类型:")),
        SP(wxT("Select Type"),   wxT("选择类型")),
        SP(wxT("Add App"),       wxT("添加应用")),
        SP(wxT("Add Function"),  wxT("添加功能")),
        SP(wxT("Cancel"),        wxT("取消")),
        SP(wxT("Are you sure you want to delete \"%s\"?"),
            wxT("确定要删除 \"%s\" 吗?")),
        SP(wxT("Confirm Delete"), wxT("确认删除")),
        SP(wxT("A simple app launcher tool"), wxT("一个简单的应用启动器")),
        SP(wxT("Confirm"),       wxT("确认")),
        // AppConfigDialog
        SP(wxT("App Config"),    wxT("应用配置")),
        SP(wxT("App Name"),      wxT("应用名称")),
        SP(wxT("Icon Path"),     wxT("图标路径")),
        SP(wxT("Select Icon"),   wxT("选择图标")),
        SP(wxT("Select App Icon"), wxT("选择应用图标")),
        SP(wxT("App name cannot be empty"), wxT("应用名称不能为空")),
        SP(wxT("Icon file does not exist"), wxT("图标文件不存在")),
        SP(wxT("Notice"),        wxT("提示")),
        // FuncConfigDialog
        SP(wxT("Function Config"), wxT("功能配置")),
        SP(wxT("Function Name"),  wxT("功能名称")),
        SP(wxT("Command List"),   wxT("命令列表")),
        SP(wxT("Enter Command"),  wxT("输入命令")),
        SP(wxT("Delete Command"), wxT("删除命令")),
        SP(wxT("Select Function Icon"), wxT("选择功能图标")),
        SP(wxT("Enter the command to execute (e.g. notepad.exe, calc.exe):"),
            wxT("输入要执行的命令 (例如 notepad.exe, calc.exe):")),
        SP(wxT("This command already exists"), wxT("该命令已存在")),
        SP(wxT("Function name cannot be empty"), wxT("功能名称不能为空")),
    });
    return loader;
}

class WiseExecApp : public wxApp
{
    wxLocale m_locale;

public:
    bool OnInit() override
    {
        if (!wxApp::OnInit())
            return false;

        wxInitAllImageHandlers();

        // Determine language from config
        wxString lang = LoadLanguageFromConfig();
        if (lang.IsEmpty()) {
            lang = (wxLocale::GetSystemLanguage() == wxLANGUAGE_CHINESE_SIMPLIFIED)
                       ? wxT("zh_CN") : wxT("en");
        }

        // Initialize wxLocale first (it creates default wxTranslations internally)
        wxLanguage wxLang = (lang == wxT("zh_CN")) ? wxLANGUAGE_CHINESE_SIMPLIFIED : wxLANGUAGE_ENGLISH;
        m_locale.Init(wxLang);

        // Now set up embedded translations AFTER wxLocale, so we don't get overwritten
        wxTranslations* trans = wxTranslations::Get();
        if (!trans) {
            trans = new wxTranslations();
            wxTranslations::Set(trans);
        }
        trans->SetLoader(CreateTranslationLoader());
        trans->SetLanguage(lang);
        trans->AddCatalog(wxT("WiseExec"));

        MainWindow* frame = new MainWindow();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(WiseExecApp);

