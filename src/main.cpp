#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "appitem.h"

const QString CONFIG_FILE_PATH = "./config.json";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "QuickStart_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    QFile styleFile(":/resources/style.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        a.setStyleSheet(styleFile.readAll());
        styleFile.close();
    }

    MainWindow w;
    w.show();

    return a.exec();
}

