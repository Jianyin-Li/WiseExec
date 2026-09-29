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
    explicit FuncItem(const wxString& name, const wxString& iconPath,
                      const std::vector<wxString>& cmds,
                      const wxString& exePath = wxEmptyString);

    wxString getName() const { return m_name; }
    void setName(const wxString& name) { m_name = name; }

    wxString getIconPath() const { return m_iconPath; }
    void setIconPath(const wxString& path) { m_iconPath = path; }

    // Executable the icon is extracted from. Empty means "derive it from
    // this function's own command list".
    wxString getExePath() const { return m_exePath; }
    void setExePath(const wxString& path) { m_exePath = path; }

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
    wxString m_exePath;
    std::vector<wxString> m_cmds;
};

#endif // FUNCITEM_H