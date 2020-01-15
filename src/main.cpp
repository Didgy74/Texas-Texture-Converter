#include <iostream>

#include <QApplication>

#include "MainTexasWindow.hpp"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Nils Petter Skålerud");
    QCoreApplication::setApplicationName("Texas Texture Converter");
    QCoreApplication::setApplicationVersion("0.1");

    MainTexasWindow* mainWindow = new MainTexasWindow;

    mainWindow->show();

    return app.exec();
}