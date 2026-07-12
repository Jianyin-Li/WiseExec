#ifndef FUNCITEM_H
#define FUNCITEM_H

#include <wx/string.h>
#include <wx/bitmap.h>
#include <vector>
#include <yaml-cpp/yaml.h>

class FuncItem
{
public:
    explicit FuncItem();
    explicit FuncItem(const wxString& name, const wxString& iconPath, const std::vector<wxString>& cmds);

    wxString getName() const { return m_name; }
    void setName(const wxString& name) { m_name = name; }

    wxString getIconPath() const { return m_iconPath; }
    void setIconPath(const wxString& path) { m_iconPath = path; }

    wxBitmap getIcon(int size = 64) const;

    std::vector<wxString> getCmds() const { return m_cmds; }
    void setCmds(const std::vector<wxString>& cmds) { m_cmds = cmds; }
    void addCmd(const wxString& cmd);
    void removeCmd(int index);

    YAML::Node toYaml() const;
    void fromYaml(const YAML::Node& node);

private:
    wxString m_name;
    wxString m_iconPath;
    std::vector<wxString> m_cmds;
};

#endif // FUNCITEM_H