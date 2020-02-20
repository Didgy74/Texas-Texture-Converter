#pragma once

#include <QWidget>
#include <QImage>

#include "Texas/Texture.hpp"

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
            QString const& fullPath,
            Texas::Texture&& texture);

    public slots:
        void floatVisualizationModeChanged(int i);
        void mipLevelSpinBoxChanged(int i);
        void mipLevelSliderChanged(int i);
        void scaleToMipChanged(int i);
        void arrayLayerSpinBoxChanged(int i);
        void arrayLayerSliderChanged(int i);

        void exportAsKTX();

    signals:

    private:
        void createLeftPanel(QLayout* parentLayout, QString const& fullPath, bool enableControls);
        void createFloatVisualizationControls(QLayout* parentLayout);
        void createMipControls(QLayout* parentLayout);
        void createArrayControls(QLayout* parentLayout);
        void createDetailsBox(QLayout* parentLayout);
        void updateImage(std::uint64_t mipIndex, std::uint64_t arrayIndex, bool scaleMipToBase);

        unsigned int getCurrentMipLevel() const;
        bool getScaleMipToBase() const;
        unsigned int getCurrentArrayLayer() const;

        QSpinBox* mipSelectorSpinBox = nullptr;
        QSlider* mipSelectorSlider = nullptr;
        QLabel* mipWidthLabel = nullptr;
        QLabel* mipHeightLabel = nullptr;
        QLabel* mipDepthLabel = nullptr;
        QCheckBox* scaleMipToBaseCheckBox = nullptr;

        QSpinBox* arraySelectorSpinBox = nullptr;
        QSlider* arraySelectorSlider = nullptr;

        QLabel* imgLabel = nullptr;
        

        Texas::Texture sourceTexture{};
        QByteArray customImgData{};
        QImage::Format qImgFormat{};
        

    };
}
