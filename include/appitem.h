#ifndef APPITEM_H
#define APPITEM_H

#include <wx/string.h>
#include <wx/bitmap.h>
#include <vector>
#include <memory>
#include <yaml-cpp/yaml.h>

class FuncItem;

class AppItem
{
public:
    explicit AppItem();
    explicit AppItem(const wxString& name, const wxString& iconPath,
                     const wxString& exePath = wxEmptyString);
    ~AppItem();

    wxString getName() const { return m_name; }
    void setName(const wxString& name) { m_name = name; }

    wxString getIconPath() const { return m_iconPath; }
    void setIconPath(const wxString& path) { m_iconPath = path; }

    // Executable this app points at. When set and no explicit icon was
    // chosen, the icon is extracted from the executable itself.
    wxString getExePath() const { return m_exePath; }
    void setExePath(const wxString& path) { m_exePath = path; }

    wxBitmap getIcon(int size = 64) const;

    const std::vector<std::shared_ptr<AppItem>>& getSubApps() const { return m_subApps; }
    const std::vector<std::shared_ptr<FuncItem>>& getFuncs() const { return m_funcs; }

    void addSubApp(std::shared_ptr<AppItem> app);
    void addFunc(std::shared_ptr<FuncItem> func);
    void removeSubApp(AppItem* app);
    void removeFunc(FuncItem* func);

    YAML::Node toYaml() const;
    void fromYaml(const YAML::Node& node);

private:
    wxString m_name;
    wxString m_iconPath;
    wxString m_exePath;
    std::vector<std::shared_ptr<AppItem>> m_subApps;
    std::vector<std::shared_ptr<FuncItem>> m_funcs;
};

#endif // APPITEM_H