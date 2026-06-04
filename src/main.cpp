#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include "appitem.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator *appTranslator = new QTranslator(&a);
    if (appTranslator->load(":/i18n/QuickStart_en")) {
        a.installTranslator(appTranslator);
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

