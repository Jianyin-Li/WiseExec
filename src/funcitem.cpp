#include "funcitem.h"
#include "icongenerator.h"
#include <wx/filename.h>

FuncItem::FuncItem()
{
}

FuncItem::FuncItem(const wxString& name, const wxString& iconPath, const std::vector<wxString>& cmds)
    : m_name(name), m_iconPath(iconPath), m_cmds(cmds)
{
}

wxBitmap FuncItem::getIcon(int size) const
{
    if (!m_iconPath.IsEmpty()) {
        wxFileName fn(m_iconPath);
        if (fn.Exists()) {
            wxImage img(m_iconPath);
            if (img.IsOk()) {
                int w = img.GetWidth();
                int h = img.GetHeight();
                int sz = (w > h) ? w : h;
                img.Rescale(sz, sz, wxIMAGE_QUALITY_HIGH);
                if (sz > size)
                    img.Rescale(size, size, wxIMAGE_QUALITY_HIGH);
                return wxBitmap(img, 32);
            }
        }
    }
    return IconGenerator::generateDefaultIcon(m_name, size);
}

void FuncItem::addCmd(const wxString& cmd)
{
    if (!cmd.IsEmpty()) {
        for (const auto& c : m_cmds) {
            if (c == cmd) return;
        }
        m_cmds.push_back(cmd);
    }
}

void FuncItem::removeCmd(int index)
{
    if (index >= 0 && index < static_cast<int>(m_cmds.size())) {
        m_cmds.erase(m_cmds.begin() + index);
    }
}

YAML::Node FuncItem::toYaml() const
{
    YAML::Node node;
    node["name"] = m_name.ToStdString();
    node["iconPath"] = m_iconPath.ToStdString();
    for (const auto& cmd : m_cmds) {
        node["cmds"].push_back(cmd.ToStdString());
    }
    return node;
}

void FuncItem::fromYaml(const YAML::Node& node)
{
    if (node["name"])
        m_name = wxString::FromUTF8(node["name"].as<std::string>().c_str());
    if (node["iconPath"])
        m_iconPath = wxString::FromUTF8(node["iconPath"].as<std::string>().c_str());

    m_cmds.clear();
    if (node["cmds"] && node["cmds"].IsSequence()) {
        for (const auto& cmd : node["cmds"]) {
            m_cmds.push_back(wxString::FromUTF8(cmd.as<std::string>().c_str()));
        }
    }
}