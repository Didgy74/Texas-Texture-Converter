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

MainTexasWindow::MainTexasWindow() : QMainWindow()
{
    QMenu* fileMenu = new QMenu;
    fileMenu->setTitle("File");

    QAction* openAct = fileMenu->addAction("Open file", this, &MainTexasWindow::openFile);
    openAct->setShortcut(QKeySequence::Open);

    this->menuBar()->addMenu(fileMenu);

    this->resize(1280, 720);
    this->setMinimumSize(QSize(800, 600));

    QWidget* outerWidget = new QWidget;
    this->setCentralWidget(outerWidget);
    QHBoxLayout* outerLayout = new QHBoxLayout;
    outerWidget->setLayout(outerLayout);
    outerLayout->setMargin(0);

    this->mainTabWidget = new QTabWidget(outerWidget);
    outerLayout->addWidget(this->mainTabWidget);
    this->mainTabWidget->setTabsClosable(true);
}

void MainTexasWindow::openFile()
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
        this->mainTabWidget->addTab(imageTabWidget, fileInfo.fileName());
    }
}