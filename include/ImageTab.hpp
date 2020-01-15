#pragma once

#include <QWidget>
#include <QByteArray>
#include <QImage>

#include "Texas/MetaData.hpp"

class QHBoxLayout;
class QGroupBox;
class QLabel;
class QSpinBox;
class QCheckBox;

namespace TexasGUI
{
    class ImageTab : public QWidget
    {
        Q_OBJECT

    public:
        ImageTab(
            const Texas::MetaData& metaData, 
            const QString& fullPath, 
            QByteArray&& rawImgData);

    public slots:
        void mipLevelChanged(int i);
        void arrayLayerChanged(int i);
        void scaleToMipChanged(int i);

    signals:

    private:
        void setupImageDetails(QGroupBox* outerBox, const QString& fullPath);
        void updateImage(std::uint64_t mipIndex, std::uint64_t arrayIndex, bool scaleMipToBase);

        QSpinBox* mipSelectorSpinBox = nullptr;
        QLabel* mipWidthLabel = nullptr;
        QLabel* mipHeightLabel = nullptr;
        QLabel* mipDepthLabel = nullptr;
        QCheckBox* scaleMipToBaseCheckBox = nullptr;

        QSpinBox* arraySelectorSpinBox = nullptr;

        QLabel* imgLabel = nullptr;
        Texas::MetaData imgMetaData{};
        QByteArray imgRawData{};
        QImage img{};
        QImage::Format img_qtFormat = QImage::Format_Invalid;
    };
}
