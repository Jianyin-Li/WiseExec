#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

namespace AppConfig {
    // 应用程序信息
    const QString APP_NAME = "QuickStart";
    const QString APP_DISPLAY_NAME = "应用启动器";
    const QString APP_VERSION = "2.0.0";
    const QString APP_BUILD_DATE = __DATE__;
    const QString APP_BUILD_TIME = __TIME__;
    
    // 作者和版权信息
    const QString APP_AUTHOR = "Jianyin";
    const QString APP_COPYRIGHT = "Copyright © 2024 All Rights Reserved.";
    const QString APP_DESCRIPTION = "一个简单的应用启动器工具";
    
    // 配置文件路径
    const QString CONFIG_FILE_PATH = "./config.json";
    
    // 资源路径
    const QString APP_ICON_PATH = ":/app_icon.ico";
    
    // 关于对话框HTML内容
    inline QString aboutHtml() {
        return QString(
            "<h3>%1</h3>"
            "<p>版本: %2</p>"
            "<p>编译时间: %3 %4</p>"
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