#pragma once

#include <QWidget>
#include <QByteArray>
#include <QImage>

#include "Texas/MetaData.hpp"

class QHBoxLayout;
class QLabel;
class QSpinBox;
class QCheckBox;
class QSlider;

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
        void mipLevelSpinBoxChanged(int i);
        void mipLevelSliderChanged(int i);
        void scaleToMipChanged(int i);
        void arrayLayerSpinBoxChanged(int i);
        void arrayLayerSliderChanged(int i);

    signals:

    private:
        void createLeftPanel(QLayout* parentLayout, const QString& fullPath);
        void createMipControls(QLayout* parentLayout);
        void createArrayControls(QLayout* parentLayout);
        void createDetailsBox(QLayout* parentLayout);
        void updateImage(std::uint64_t mipIndex, std::uint64_t arrayIndex, bool scaleMipToBase);

        QSpinBox* mipSelectorSpinBox = nullptr;
        QSlider* mipSelectorSlider = nullptr;
        QLabel* mipWidthLabel = nullptr;
        QLabel* mipHeightLabel = nullptr;
        QLabel* mipDepthLabel = nullptr;
        QCheckBox* scaleMipToBaseCheckBox = nullptr;

        QSpinBox* arraySelectorSpinBox = nullptr;
        QSlider* arraySelectorSlider = nullptr;

        QLabel* imgLabel = nullptr;
        Texas::MetaData imgMetaData{};
        QByteArray imgRawData{};
        QImage img{};
        QImage::Format img_qtFormat = QImage::Format_Invalid;
    };
}
