#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

namespace AppConfig {
    const QString APP_NAME = "QuickStart";
    const QString APP_DISPLAY_NAME = "QuickStart";
    const QString APP_VERSION = "2.0.0";
    const QString APP_BUILD_DATE = __DATE__;
    const QString APP_BUILD_TIME = __TIME__;

    const QString APP_AUTHOR = "Jianyin";
    const QString APP_COPYRIGHT = "Copyright (C) 2024 All Rights Reserved.";
    const QString APP_DESCRIPTION = "A simple app launcher tool";

    const QString CONFIG_FILE_PATH = "./config.json";

    const QString APP_ICON_PATH = ":/app_icon.ico";

    inline QString aboutHtml() {
        return QString(
            "<h3>%1</h3>"
            "<p>Version: %2</p>"
            "<p>Build: %3 %4</p>"
            "<p>%5</p>"
            "<p>%6</p>")
            .arg(APP_DISPLAY_NAME)
            .arg(APP_VERSION)
            .arg(APP_BUILD_DATE)
            .arg(APP_BUILD_TIME)
            .arg(APP_DESCRIPTION)
            .arg(APP_COPYRIGHT);
    }
}

#endif // CONFIG_H