#include "ImageTab.hpp"

#include <QBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QtMath>

#include <iostream>

#include "Texas/Tools.hpp"

namespace TexasGUI::detail
{
	QImage::Format toQImageFormat(Texas::PixelFormat pFormat)
	{
		switch (pFormat)
		{
		case Texas::PixelFormat::RGB_8:
			return QImage::Format_RGB888;
		case Texas::PixelFormat::RGBA_8:
			return QImage::Format_RGBA8888;
		default:
			return QImage::Format::Format_Invalid;
		}
	}
}

TexasGUI::ImageTab::ImageTab(
	const Texas::MetaData& metaData, 
	const QString& fullPath,
	QByteArray&& rawImgData) :
	QWidget(),
	imgMetaData(metaData),
	imgRawData(static_cast<QByteArray&&>(rawImgData)),
	img_qtFormat(detail::toQImageFormat(imgMetaData.pixelFormat))
{
	QHBoxLayout* outerLayout = new QHBoxLayout;
	this->setLayout(outerLayout);

	QGroupBox* detailBox = new QGroupBox;
	outerLayout->addWidget(detailBox);

	this->setupImageDetails(detailBox, fullPath);

	QScrollArea* imageScrollArea = new QScrollArea;
	outerLayout->addWidget(imageScrollArea);

	this->imgLabel = new QLabel;
	imageScrollArea->setWidget(this->imgLabel);

	updateImage(0, 0, false);
}

void TexasGUI::ImageTab::setupImageDetails(QGroupBox* outerBox, const QString& fullPath)
{
	outerBox->setTitle("Image properties");
	outerBox->setMinimumWidth(200);
	outerBox->setMaximumWidth(200);

	QVBoxLayout* outerVLayout = new QVBoxLayout;
	outerBox->setLayout(outerVLayout);
	outerVLayout->setSpacing(0);

	// Setup base dimensions box
	{
		QGroupBox* baseDimensionsBox = new QGroupBox;
		outerVLayout->addWidget(baseDimensionsBox);
		baseDimensionsBox->setTitle("Base dimensions");

		QVBoxLayout* innerVLayout = new QVBoxLayout;
		baseDimensionsBox->setLayout(innerVLayout);
		
		// Width label
		QLabel* baseWidthLabel = new QLabel;
		innerVLayout->addWidget(baseWidthLabel);
		baseWidthLabel->setText(QString("Width: ") + QString::number(this->imgMetaData.baseDimensions.width));

		// Height label
		QLabel* baseHeightLabel = new QLabel;
		innerVLayout->addWidget(baseHeightLabel);
		baseHeightLabel->setText(QString("Height: ") + QString::number(this->imgMetaData.baseDimensions.height));

		// Depth label
		QLabel* baseDepthLabel = new QLabel;
		innerVLayout->addWidget(baseDepthLabel);
		baseDepthLabel->setText(QString("Depth: ") + QString::number(this->imgMetaData.baseDimensions.depth));
	}

	// Setup mip dimensions box
	{
		QGroupBox* mipBox = new QGroupBox;
		outerVLayout->addWidget(mipBox);
		mipBox->setTitle("Mip");

		QVBoxLayout* innerVLayout = new QVBoxLayout;
		mipBox->setLayout(innerVLayout);

		// Create the mip selector line
		{
			QWidget* levelSelectorContainer = new QWidget;
			innerVLayout->addWidget(levelSelectorContainer);
			QHBoxLayout* levelSelectorLayout = new QHBoxLayout;
			levelSelectorContainer->setLayout(levelSelectorLayout);
			levelSelectorLayout->setMargin(0);

			QLabel* mipTextLabel = new QLabel;
			levelSelectorLayout->addWidget(mipTextLabel);
			mipTextLabel->setText("Level: ");

			this->mipSelectorSpinBox = new QSpinBox;
			levelSelectorLayout->addWidget(this->mipSelectorSpinBox);
			this->mipSelectorSpinBox->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
			this->mipSelectorSpinBox->setMaximum(this->imgMetaData.mipLevelCount - 1);
			QObject::connect(this->mipSelectorSpinBox, SIGNAL(valueChanged(int)), this, SLOT(mipLevelChanged(int)));

			QLabel* maxMipTextLabel = new QLabel;
			levelSelectorLayout->addWidget(maxMipTextLabel);
			maxMipTextLabel->setText(QString("/ ") + QString::number(this->imgMetaData.mipLevelCount - 1));
		}

		Texas::Dimensions mipDims = Texas::Tools::calcMipmapDimensions(this->imgMetaData.baseDimensions, 0);

		this->mipWidthLabel = new QLabel;
		innerVLayout->addWidget(this->mipWidthLabel);
		this->mipWidthLabel->setText(QString("Width: ") + QString::number(mipDims.width));

		this->mipHeightLabel = new QLabel;
		innerVLayout->addWidget(this->mipHeightLabel);
		this->mipHeightLabel->setText(QString("Height: ") + QString::number(mipDims.height));

		this->mipDepthLabel = new QLabel;
		innerVLayout->addWidget(this->mipDepthLabel);
		mipDepthLabel->setText(QString("Depth: ") + QString::number(mipDims.depth));

		// Make the "Scale to base" line
		{
			QWidget* scaleMipToBaseContainer = new QWidget;
			innerVLayout->addWidget(scaleMipToBaseContainer);
			QHBoxLayout* scaleMipToBaseLayout = new QHBoxLayout;
			scaleMipToBaseContainer->setLayout(scaleMipToBaseLayout);
			scaleMipToBaseLayout->setMargin(0);

			QLabel* scaleMipToBaseLabel = new QLabel;
			scaleMipToBaseLayout->addWidget(scaleMipToBaseLabel);
			scaleMipToBaseLabel->setText("Scale to base: ");

			this->scaleMipToBaseCheckBox = new QCheckBox;
			scaleMipToBaseLayout->addWidget(this->scaleMipToBaseCheckBox);
			this->scaleMipToBaseCheckBox->setLayoutDirection(Qt::LeftToRight);
			QObject::connect(this->scaleMipToBaseCheckBox, SIGNAL(stateChanged(int)), this, SLOT(scaleToMipChanged(int)));
		}
	}

	// Add array layer controller line
	{
		QWidget* levelSelectorContainer = new QWidget;
		outerVLayout->addWidget(levelSelectorContainer);
		QHBoxLayout* levelSelectorLayout = new QHBoxLayout;
		levelSelectorContainer->setLayout(levelSelectorLayout);
		levelSelectorLayout->setMargin(0);

		QLabel* layerTextLabel = new QLabel;
		levelSelectorLayout->addWidget(layerTextLabel);
		layerTextLabel->setText("Layer: ");

		this->arraySelectorSpinBox = new QSpinBox;
		levelSelectorLayout->addWidget(this->arraySelectorSpinBox);
		this->arraySelectorSpinBox->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		this->arraySelectorSpinBox->setMaximum(this->imgMetaData.arrayLayerCount - 1);
		QObject::connect(this->arraySelectorSpinBox, SIGNAL(valueChanged(int)), this, SLOT(arrayLayerChanged(int)));

		QLabel* maxMipTextLabel = new QLabel;
		levelSelectorLayout->addWidget(maxMipTextLabel);
		maxMipTextLabel->setText(QString("/ ") + QString::number(this->imgMetaData.arrayLayerCount - 1));
	}

	QSpacerItem* endOfImageDetailsSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	outerVLayout->addSpacerItem(endOfImageDetailsSpacer);
}

void TexasGUI::ImageTab::mipLevelChanged(int i)
{
	std::uint64_t arrayIndex = this->arraySelectorSpinBox->value();
	Qt::CheckState scaleMipToBaseState = this->scaleMipToBaseCheckBox->checkState();
	bool scaleMipToBase = scaleMipToBaseState == Qt::Checked ? true : false;

	updateImage(i, arrayIndex, scaleMipToBase);
}

void TexasGUI::ImageTab::arrayLayerChanged(int i)
{
	std::uint64_t mipLevel = this->mipSelectorSpinBox->value();

	Qt::CheckState scaleMipToBaseState = this->scaleMipToBaseCheckBox->checkState();
	bool scaleMipToBase = scaleMipToBaseState == Qt::Checked ? true : false;

	updateImage(mipLevel, i, scaleMipToBase);
}

void TexasGUI::ImageTab::scaleToMipChanged(int i)
{
	std::uint64_t arrayIndex = this->arraySelectorSpinBox->value();
	std::uint64_t mipLevel = this->mipSelectorSpinBox->value();

	Qt::CheckState scaleMipToBaseState = static_cast<Qt::CheckState>(i);
	bool scaleMipToBase = scaleMipToBaseState == Qt::Checked ? true : false;

	updateImage(mipLevel, arrayIndex, scaleMipToBase);
}

void TexasGUI::ImageTab::updateImage(std::uint64_t mipIndex, std::uint64_t arrayIndex, bool scaleMipToBase)
{
	const uchar* imgDataStart = reinterpret_cast<const uchar*>(this->imgRawData.constData());
	imgDataStart += Texas::Tools::calcArrayLayerOffset(this->imgMetaData, mipIndex, arrayIndex).value();

	const Texas::Dimensions mipDims = Texas::Tools::calcMipmapDimensions(this->imgMetaData.baseDimensions, mipIndex);

	this->mipWidthLabel->setText(QString("Width: ") + QString::number(mipDims.width));
	this->mipHeightLabel->setText(QString("Height: ") + QString::number(mipDims.height));
	this->mipDepthLabel->setText(QString("Depth: ") + QString::number(mipDims.depth));

	this->img = QImage((const uchar*)imgDataStart, mipDims.width, mipDims.height, img_qtFormat);
	QPixmap temp = QPixmap::fromImage(this->img);

	if (scaleMipToBase == false || mipIndex == 0)
	{
		this->imgLabel->setPixmap(temp);
		this->imgLabel->adjustSize();
	}
	else
	{
		QSize size = qPow(2, mipIndex) * temp.size();
		QPixmap scaledImage = temp.scaled(size);
		this->imgLabel->setPixmap(scaledImage);
		this->imgLabel->adjustSize();
	}
}
