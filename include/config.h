#ifndef CONFIG_H
#define CONFIG_H

#include <wx/string.h>

namespace AppConfig {
    inline const wxChar* APP_NAME = wxT("WiseExec");
    inline const wxChar* APP_DISPLAY_NAME = wxT("WiseExec");
    inline const wxChar* APP_VERSION = wxT("2.0.0");
    inline const wxChar* APP_AUTHOR = wxT("Jianyin");
    inline const wxChar* APP_COPYRIGHT = wxT("Copyright (C) 2024 All Rights Reserved.");
    inline const wxChar* APP_DESCRIPTION = wxT("A simple app launcher tool");

    inline const wxChar* CONFIG_FILE_PATH = wxT("./config.json");
    inline const wxChar* CONFIG_FILE_PATH_YAML = wxT("./config.yaml");

    inline const wxChar* APP_ICON_PATH = wxT("resources/app_icon.ico");

    inline wxString aboutHtml() {
        wxString html;
        html << wxT("<h3>") << APP_DISPLAY_NAME << wxT("</h3>");
        html << wxT("<p>Version: ") << APP_VERSION << wxT("</p>");
        html << wxT("<p>") << APP_DESCRIPTION << wxT("</p>");
        html << wxT("<p>") << APP_COPYRIGHT << wxT("</p>");
        return html;
    }
}

#endif // CONFIG_H