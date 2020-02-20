#include "MainTexasWindow.hpp"

#include "ImageTab.hpp"
#include "TexasGUI/Utilities.hpp"

#include "Texas/Texas.hpp"
#include "Texas/Tools.hpp"

#include <QBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>
#include <QGroupBox>
#include <QScrollArea>
#include <QLabel>
#include <QTabBar>
#include <QSpacerItem>
#include <QStackedLayout>

TexasGUI::MainTexasWindow::MainTexasWindow() 
{
    this->resize(1280, 720);
    this->setMinimumSize(QSize(800, 600));


    QMenu* fileMenu = new QMenu;
    this->menuBar()->addMenu(fileMenu);
    fileMenu->setTitle("File");
    QAction* openAct = fileMenu->addAction("Open file", this, &MainTexasWindow::openFile);
    openAct->setShortcut(QKeySequence::Open);
    QAction* quitAct = fileMenu->addAction("Quit", this, &MainTexasWindow::clickedMenuQuit);
    quitAct->setShortcut(QKeySequence::Quit);


    QWidget* centerWidget = new QWidget;
    setCentralWidget(centerWidget);
    centerWidget->setObjectName("centerWidget");

    QVBoxLayout* centerLayout = new QVBoxLayout;
    centerWidget->setLayout(centerLayout);
    centerLayout->setMargin(0);

    // Make tab widget line
    {
        QWidget* tabBarLine = new QWidget;
        centerLayout->addWidget(tabBarLine);

        QHBoxLayout* hLayout = new QHBoxLayout;
        tabBarLine->setLayout(hLayout);
        hLayout->setMargin(0);
        hLayout->setSpacing(0);

        this->tabBar = new QTabBar;
        hLayout->addWidget(tabBar);
        this->tabBar->setExpanding(false);
        this->tabBar->setMovable(true);
        this->tabBar->setDrawBase(false);
        this->tabBar->setTabsClosable(true);
        this->tabBar->setMinimumWidth(0);
        QObject::connect(this->tabBar, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
        QObject::connect(this->tabBar, SIGNAL(currentChanged(int)), this, SLOT(tabSelectedChanged(int)));
        QObject::connect(this->tabBar, SIGNAL(tabMoved(int, int)), this, SLOT(tabMoved(int, int)));

        QWidget* noTabSpace = new QWidget;
        hLayout->addWidget(noTabSpace);
        noTabSpace->setObjectName("noTabSpace");
        noTabSpace->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    }


    this->tabsStackWidget = new QWidget;
    centerLayout->addWidget(this->tabsStackWidget);
    this->tabsStackLayout = new QStackedLayout;
    this->tabsStackWidget->setLayout(this->tabsStackLayout);
    this->tabsStackLayout->setMargin(0);


    /*    QTabBar* tabBar = new QTabBar;
    outerLayout->addWidget(tabBar);
    tabBar->setMovable(true);
    tabBar->setTabsClosable(true);
    tabBar->setExpanding(false);
    tabBar->setDrawBase(false);

    tabBar->addTab("Tab name");
    tabBar->addTab("Test");

  */
  
}

void TexasGUI::MainTexasWindow::openFile()
{
    QString fileFilter = "Images (*.png *.ktx)";
    
    QFileDialog fileDialog = QFileDialog(this, "Open an image file", QString(), fileFilter);

    int dialogResult = fileDialog.exec();

    if (dialogResult == QDialog::Accepted)
    {
        QString fileName = fileDialog.selectedFiles().first();
        QFile file = QFile(fileName);
        file.open(QFile::ReadOnly);
        if (!file.isOpen())
        {
            QMessageBox msgBox;
            msgBox.setText("Unable to open this file.");
            msgBox.exec();
            return;
        }

        QByteArray fileBuffer = file.readAll();

        Texas::ConstByteSpan fileBufferSpan = { reinterpret_cast<const std::byte*>(fileBuffer.constData()), static_cast<std::size_t>(fileBuffer.size()) };
        Texas::ResultValue<Texas::Texture> loadResult = Texas::loadFromBuffer(fileBufferSpan);
        if (!loadResult.isSuccessful())
        {
            // We couldnt load this file
            TexasGUI::Utils::displayErrorBox("Unable to load this file.", loadResult.errorMessage());
            return;
        }
        
        Texas::Texture texture = static_cast<Texas::Texture&&>(loadResult.value());

        TexasGUI::ImageTab* imageTabWidget = new TexasGUI::ImageTab(file.fileName(), static_cast<Texas::Texture&&>(texture));

        int newIndex = this->tabsStackLayout->addWidget(imageTabWidget);

        QFileInfo fileInfo = QFileInfo(file);
        tabBar->addTab(fileInfo.fileName());
        this->tabsStackLayout->setCurrentIndex(newIndex);
        this->tabBar->setCurrentIndex(newIndex);
    }
}

void TexasGUI::MainTexasWindow::clickedMenuQuit()
{
    this->close();
}

void TexasGUI::MainTexasWindow::tabCloseRequested(int index)
{
    this->tabsStackLayout->removeItem(this->tabsStackLayout->itemAt(index));
    this->tabBar->removeTab(index);
}

void TexasGUI::MainTexasWindow::tabSelectedChanged(int index)
{
    this->tabsStackLayout->setCurrentIndex(index);
}

void TexasGUI::MainTexasWindow::tabMoved(int from, int to)
{
    QWidget* fromWidget = this->tabsStackLayout->itemAt(from)->widget();
    QWidget* toWidget = this->tabsStackLayout->itemAt(to)->widget();

    this->tabsStackLayout->insertWidget(from, toWidget);
    this->tabsStackLayout->insertWidget(to, fromWidget);
}
