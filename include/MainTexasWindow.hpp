#pragma once

#include <QMainWindow>
#include <QList>

class QTabBar;
class QStackedLayout;

namespace TexasGUI
{
    class ImageTab;

    class MainTexasWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        MainTexasWindow();


    public slots:
        void clickedMenuQuit();
        void openFile();
        void tabCloseRequested(int index);
        void tabSelectedChanged(int index);
        void tabMoved(int from, int to);

    signals:

    private:

        QWidget* tabsStackWidget = nullptr;
        QStackedLayout* tabsStackLayout = nullptr;
        QList<ImageTab*> imageTabWidgets;

        QTabBar* tabBar = nullptr;
    };
}

