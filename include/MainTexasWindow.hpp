#pragma once

#include <QMainWindow>

class QTabWidget;

class MainTexasWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainTexasWindow();
        

public slots:
    void openFile();

    signals:

private:
    void setImage(const QImage& newImage);
    bool loadFile(const QString&);

    QTabWidget* mainTabWidget = nullptr;
};
