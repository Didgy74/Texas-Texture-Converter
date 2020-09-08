#pragma once

#include <QWidget>
#include <QImage>
#include <QLabel>

#include "Texas/Texture.hpp"

class QHBoxLayout;
class QLabel;
class QSpinBox;
class QCheckBox;
class QSlider;

namespace TexasGUI
{
  struct MinMaxData
  {
    enum class Type
    {
      Int,
      UnsignedInt,
      Float
    };
    Type type{};
    struct A
    {
      std::int64_t max_int64[4];
      std::int64_t min_int64[4];
      std::uint64_t max_uint64[4];
      std::uint64_t min_uint64[4];
      double max_float64[4];
      double min_float64[4];
    };
    struct B
    {
      std::vector<A> layers;
    };
    std::vector<B> mipLevels;
  };

  struct MinMaxLabels
  {
    QLabel* min[4] = {};
    QLabel* max[4] = {};
  };

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
      void createMinMaxBox(QLayout* parentLayout);
      void createDetailsBox(QLayout* parentLayout);

      void updateMinMaxLabels(uint8_t mipIndex, uint64_t layerIndex);
      void updateImage(std::uint8_t mipIndex, std::uint64_t arrayIndex, bool scaleMipToBase);

      unsigned int getCurrentMipLevel() const;
      bool getScaleMipToBase() const;
      unsigned int getCurrentArrayLayer() const;

      MinMaxData minMaxData{};

      QSpinBox* mipSelectorSpinBox = nullptr;
      QSlider* mipSelectorSlider = nullptr;
      QLabel* mipWidthLabel = nullptr;
      QLabel* mipHeightLabel = nullptr;
      QLabel* mipDepthLabel = nullptr;
      QCheckBox* scaleMipToBaseCheckBox = nullptr;

      QSpinBox* arraySelectorSpinBox = nullptr;
      QSlider* arraySelectorSlider = nullptr;

      MinMaxLabels minMaxLabels{};

      QLabel* imgLabel = nullptr;
        

      Texas::Texture sourceTexture{};
      QByteArray customImgData{};
      QImage::Format qImgFormat{};
        

  };
}
