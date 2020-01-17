#include <iostream>

#include <QApplication>

#include <fstream>
#include <string>
#include <iostream>

#include "MainTexasWindow.hpp"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Nils Petter Skålerud");
    QCoreApplication::setApplicationName("Texas Texture Converter");
    QCoreApplication::setApplicationVersion("0.1");
    
    std::ifstream file("Dark stylesheet.txt", std::fstream::ate);
    if (!file.is_open())
    {
        std::cout << "Could not open file." << std::endl;
        std::abort();
    }

    std::size_t fileSize = file.tellg();
    file.seekg(0);

    std::string fileString;
    fileString.resize(fileSize, ' ');

    file.read(fileString.data(), fileSize);

    qApp->setStyleSheet(QString::fromStdString(fileString));
    

    TexasGUI::MainTexasWindow* mainWindow = new TexasGUI::MainTexasWindow;

    mainWindow->show();

    return app.exec();
}