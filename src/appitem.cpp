#include "appitem.h"
#include "funcitem.h"
#include "icongenerator.h"
#include "exeiconextractor.h"
#include "config.h"
#include <wx/filename.h>

AppItem::AppItem()
{
}

AppItem::AppItem(const wxString& name, const wxString& iconPath, const wxString& exePath)
    : m_name(name), m_iconPath(iconPath), m_exePath(exePath)
{
}

AppItem::~AppItem()
{
}

wxBitmap AppItem::getIcon(int size) const
{
    // 1. An explicitly configured icon file always wins.
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

    // 2. Otherwise extract it from the executable this app points at.
    if (AppConfig::extractExeIcon && !m_exePath.IsEmpty()) {
        wxBitmap bmp = ExeIconExtractor::Extract(m_exePath, size);
        if (bmp.IsOk())
            return bmp;
    }

    // 3. Fall back to the first function below whose own command resolves to
    //    an icon, so a folder whose entries are all real programs still gets
    //    a representative icon. A function only ever looks at its own
    //    command list, never at its siblings.
    if (AppConfig::extractExeIcon) {
        for (const auto& func : m_funcs) {
            if (!func)
                continue;
            for (const wxString& cmd : func->getCmds()) {
                wxBitmap bmp = ExeIconExtractor::Extract(cmd, size);
                if (bmp.IsOk())
                    return bmp;
            }
        }
    }

    return IconGenerator::generateDefaultIcon(m_name, size);
}

void AppItem::addSubApp(std::shared_ptr<AppItem> app)
{
    if (app) {
        m_subApps.push_back(app);
    }
}

void AppItem::addFunc(std::shared_ptr<FuncItem> func)
{
    if (func) {
        m_funcs.push_back(func);
    }
}

void AppItem::removeSubApp(AppItem* app)
{
    m_subApps.erase(
        std::remove_if(m_subApps.begin(), m_subApps.end(),
            [app](const std::shared_ptr<AppItem>& p) { return p.get() == app; }),
        m_subApps.end());
}

void AppItem::removeFunc(FuncItem* func)
{
    m_funcs.erase(
        std::remove_if(m_funcs.begin(), m_funcs.end(),
            [func](const std::shared_ptr<FuncItem>& p) { return p.get() == func; }),
        m_funcs.end());
}

YAML::Node AppItem::toYaml() const
{
    YAML::Node node;
    node["name"] = m_name.ToStdString();
    node["iconPath"] = m_iconPath.ToStdString();
    node["exePath"] = m_exePath.ToStdString();

    for (const auto& app : m_subApps) {
        node["subApps"].push_back(app->toYaml());
    }
    for (const auto& func : m_funcs) {
        node["funcs"].push_back(func->toYaml());
    }
    return node;
}

void AppItem::fromYaml(const YAML::Node& node)
{
    if (node["name"])
        m_name = wxString::FromUTF8(node["name"].as<std::string>().c_str());
    if (node["iconPath"])
        m_iconPath = wxString::FromUTF8(node["iconPath"].as<std::string>().c_str());
    if (node["exePath"])
        m_exePath = wxString::FromUTF8(node["exePath"].as<std::string>().c_str());

    m_subApps.clear();
    m_funcs.clear();

    if (node["subApps"] && node["subApps"].IsSequence()) {
        for (const auto& child : node["subApps"]) {
            auto app = std::make_shared<AppItem>();
            app->fromYaml(child);
            m_subApps.push_back(app);
        }
    }
    if (node["funcs"] && node["funcs"].IsSequence()) {
        for (const auto& child : node["funcs"]) {
            auto func = std::make_shared<FuncItem>();
            func->fromYaml(child);
            m_funcs.push_back(func);
        }
    }
}