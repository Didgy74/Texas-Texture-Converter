#include "MainTexasWindow.hpp"

#include "ImageTab.hpp"

#include "Texas/Texas.hpp"

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
        Texas::LoadResult<Texas::MemReqs> parseFileResult = Texas::getMemReqs(fileBufferSpan);
        if (!parseFileResult.isSuccessful())
        {
            QMessageBox msgBox;
            switch (parseFileResult.resultType())
            {
            case Texas::ResultType::CorruptFileData:
                msgBox.setText("This image file is corrupt.");
                break;
            case Texas::ResultType::FileNotSupported:
                msgBox.setText("Texas does not support this file.");
                break;
            default:
                break;
            }
            
            if (parseFileResult.errorMessage() != nullptr)
                msgBox.setInformativeText(parseFileResult.errorMessage());
            
            msgBox.exec();
            return;
        }

        Texas::MemReqs texMemReqs = parseFileResult.value();

        QByteArray dstImageBuffer = QByteArray(texMemReqs.memoryRequired(), 0);
        QByteArray workingMem{};

        Texas::ByteSpan texasDstBuffer = { reinterpret_cast<std::byte*>(dstImageBuffer.data()), static_cast<std::size_t>(dstImageBuffer.size()) };
        Texas::ByteSpan texasWorkingMem{};
        if (texMemReqs.workingMemoryRequired() > 0)
        {
            workingMem = QByteArray(texMemReqs.workingMemoryRequired(), 0);
            texasWorkingMem = { reinterpret_cast<std::byte*>(workingMem.data()), static_cast<std::size_t>(workingMem.size()) };
        }
        Texas::Result loadFileResult = Texas::loadImageData(texMemReqs, texasDstBuffer, texasWorkingMem);
        if (!loadFileResult.isSuccessful())
        {
            QMessageBox msgBox;
            switch (loadFileResult.resultType())
            {
            case Texas::ResultType::CorruptFileData:
                msgBox.setText("This image file is corrupt.");
                break;
            case Texas::ResultType::FileNotSupported:
                msgBox.setText("Texas does not support this file.");
                break;
            default:
                break;
            }

            if (loadFileResult.errorMessage() != nullptr)
                msgBox.setInformativeText(loadFileResult.errorMessage());

            msgBox.exec();
            return;
        }

        TexasGUI::ImageTab* imageTabWidget = new TexasGUI::ImageTab(texMemReqs.metaData(), file.fileName(), static_cast<QByteArray&&>(dstImageBuffer));

        QFileInfo fileInfo = QFileInfo(file);

        tabBar->addTab(fileInfo.fileName());
        int newIndex = this->tabsStackLayout->addWidget(imageTabWidget);
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
