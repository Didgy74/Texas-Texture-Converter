#include "ImageTab.hpp"

#include <QBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QtMath>
#include <QSlider>

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

	this->createLeftPanel(outerLayout, fullPath);

	QScrollArea* imageScrollArea = new QScrollArea;
	outerLayout->addWidget(imageScrollArea);

	this->imgLabel = new QLabel;
	imageScrollArea->setWidget(this->imgLabel);
	
	updateImage(0, 0, false);
}

void TexasGUI::ImageTab::createLeftPanel(QLayout* parentLayout, const QString& fullPath)
{
	QWidget* controlsWidget = new QWidget;
	parentLayout->addWidget(controlsWidget);
	controlsWidget->setMinimumWidth(200);
	controlsWidget->setMaximumWidth(200);

	QVBoxLayout* outerVLayout = new QVBoxLayout;
	controlsWidget->setLayout(outerVLayout);
	outerVLayout->setMargin(0);

	if (this->imgMetaData.mipLevelCount > 1)
		createMipControls(outerVLayout);

	if (this->imgMetaData.arrayLayerCount > 1)
		createArrayControls(outerVLayout);

	createDetailsBox(outerVLayout);

	QSpacerItem* endOfControlsSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	outerVLayout->addSpacerItem(endOfControlsSpacer);
}

void TexasGUI::ImageTab::createMipControls(QLayout* parentLayout)
{
	QGroupBox* mipBox = new QGroupBox;
	parentLayout->addWidget(mipBox);
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
		QObject::connect(this->mipSelectorSpinBox, SIGNAL(valueChanged(int)), this, SLOT(mipLevelSpinBoxChanged(int)));

		QLabel* maxMipTextLabel = new QLabel;
		levelSelectorLayout->addWidget(maxMipTextLabel);
		maxMipTextLabel->setText(QString("/ ") + QString::number(this->imgMetaData.mipLevelCount - 1));
	}

	// Make the mip slider
	this->mipSelectorSlider = new QSlider;
	innerVLayout->addWidget(this->mipSelectorSlider);
	this->mipSelectorSlider->setMaximum(this->imgMetaData.mipLevelCount - 1);
	this->mipSelectorSlider->setPageStep(1);
	this->mipSelectorSlider->setOrientation(Qt::Horizontal);
	this->mipSelectorSlider->setTickPosition(QSlider::TicksBelow);
	QObject::connect(this->mipSelectorSlider, SIGNAL(valueChanged(int)), this, SLOT(mipLevelSliderChanged(int)));

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

void TexasGUI::ImageTab::createArrayControls(QLayout* parentLayout)
{
	QGroupBox* arrayBox = new QGroupBox;
	parentLayout->addWidget(arrayBox);
	arrayBox->setTitle("Array");

	QVBoxLayout* innerVLayout = new QVBoxLayout;
	arrayBox->setLayout(innerVLayout);
	// Add array layer controller line
	{
		QWidget* levelSelectorContainer = new QWidget;
		innerVLayout->addWidget(levelSelectorContainer);
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
		QObject::connect(this->arraySelectorSpinBox, SIGNAL(valueChanged(int)), this, SLOT(arrayLayerSpinBoxChanged(int)));

		QLabel* maxMipTextLabel = new QLabel;
		levelSelectorLayout->addWidget(maxMipTextLabel);
		maxMipTextLabel->setText(QString("/ ") + QString::number(this->imgMetaData.arrayLayerCount - 1));
	}

	this->arraySelectorSlider = new QSlider;
	innerVLayout->addWidget(this->arraySelectorSlider);
	this->arraySelectorSlider->setMaximum(this->imgMetaData.arrayLayerCount - 1);
	this->arraySelectorSlider->setPageStep(1);
	this->arraySelectorSlider->setOrientation(Qt::Horizontal);
	this->arraySelectorSlider->setTickPosition(QSlider::TicksBelow);
	QObject::connect(this->arraySelectorSlider, SIGNAL(valueChanged(int)), this, SLOT(arrayLayerSliderChanged(int)));
}

namespace TexasGUI
{
	QString toString(Texas::TextureType in)
	{
		using namespace Texas;
		switch (in)
		{
		case TextureType::Array1D:
			return "1D Array";
		case TextureType::Array2D:
			return "2D Array";
		case TextureType::Array3D:
			return "3D Array";
		case TextureType::ArrayCubemap:
			return "Cubemap Array";
		case TextureType::Texture1D:
			return "1D";
		case TextureType::Texture2D:
			return "2D";
		case TextureType::Texture3D:
			return "3D";
		case TextureType::Cubemap:
			return "Cubemap";
		default:
			return "Error";
		}
	}

	QString toString(Texas::PixelFormat in)
	{
		using namespace Texas;
		switch (in)
		{
		case PixelFormat::R_8:
			return "R_8";
		case PixelFormat::RG_8:
			return "RG_8";
		case PixelFormat::RA_8:
			return "RA_8";
		case PixelFormat::RGB_8:
			return "RGB_8";
		case PixelFormat::RGBA_8:
			return "RGBA_8";
		default:
			return "Error";
		}
	}

	QString toString(Texas::ChannelType in)
	{
		using namespace Texas;
		switch (in)
		{
		case ChannelType::UnsignedFloat:
			return "Unsigned Float";
		case ChannelType::UnsignedInteger:
			return "Unsigned Integer";
		case ChannelType::UnsignedNormalized:
			return "Unsigned Normalized";
		case ChannelType::UnsignedScaled:
			return "Unsigned Scaled";
		case ChannelType::SignedFloat:
			return "Signed Float";
		case ChannelType::SignedInteger:
			return "Signed Integer";
		case ChannelType::SignedNormalized:
			return "Signed Normalized";
		case ChannelType::SignedScaled:
			return "Signed Scaled";
		case ChannelType::sRGB:
			return "sRGB";
		default: 
			return "Error";
		}
	}

	QString toString(Texas::ColorSpace in)
	{
		using namespace Texas;
		switch (in)
		{
		case ColorSpace::Linear:
			return "Linear";
		case ColorSpace::sRGB:
			return "sRGB";
		default:
			return "Error";
		}
	}

	QString toString(Texas::FileFormat in)
	{
		using namespace Texas;
		switch (in)
		{
		case FileFormat::KTX:
			return "KTX";
		case FileFormat::PNG:
			return "PNG";
		default:
			return "Error";
		}
	}
}

void TexasGUI::ImageTab::createDetailsBox(QLayout* parentLayout)
{
	QGroupBox* detailsBox = new QGroupBox;
	parentLayout->addWidget(detailsBox);
	detailsBox->setTitle("Image details");

	QVBoxLayout* vLayout = new QVBoxLayout;
	detailsBox->setLayout(vLayout);

	QLabel* textureTypeLabel = new QLabel;
	vLayout->addWidget(textureTypeLabel);
	textureTypeLabel->setText("Type: " + TexasGUI::toString(this->imgMetaData.textureType));

	QLabel* formatLabel = new QLabel;
	vLayout->addWidget(formatLabel);
	formatLabel->setText("Format: " + TexasGUI::toString(this->imgMetaData.pixelFormat));

	QLabel* channelTypeLabel = new QLabel;
	vLayout->addWidget(channelTypeLabel);
	channelTypeLabel->setText("Channel type: " + TexasGUI::toString(this->imgMetaData.channelType));

	QLabel* colorSpaceLabel = new QLabel;
	vLayout->addWidget(colorSpaceLabel);
	colorSpaceLabel->setText("Color space: " + TexasGUI::toString(this->imgMetaData.colorSpace));

	QLabel* widthLabel = new QLabel;
	vLayout->addWidget(widthLabel);
	widthLabel->setText("Width: " + QString::number(this->imgMetaData.baseDimensions.width));

	QLabel* heightLabel = new QLabel;
	vLayout->addWidget(heightLabel);
	heightLabel->setText("Height: " + QString::number(this->imgMetaData.baseDimensions.height));

	QLabel* depthLabel = new QLabel;
	vLayout->addWidget(depthLabel);
	depthLabel->setText("Depth: " + QString::number(this->imgMetaData.baseDimensions.depth));

	QLabel* mipCountLabel = new QLabel;
	vLayout->addWidget(mipCountLabel);
	mipCountLabel->setText("Mip level count: " + QString::number(this->imgMetaData.mipLevelCount));

	QLabel* arrayLayerLabel = new QLabel;
	vLayout->addWidget(arrayLayerLabel);
	arrayLayerLabel->setText("Array layer count: " + QString::number(this->imgMetaData.arrayLayerCount));

	QLabel* srcFileFormatLabel = new QLabel;
	vLayout->addWidget(srcFileFormatLabel);
	srcFileFormatLabel->setText("Source file-format: " + TexasGUI::toString(this->imgMetaData.srcFileFormat));
}

void TexasGUI::ImageTab::mipLevelSpinBoxChanged(int i)
{
	this->mipSelectorSlider->setValue(i);

	const Texas::Dimensions mipDims = Texas::Tools::calcMipmapDimensions(this->imgMetaData.baseDimensions, i);

	this->mipWidthLabel->setText(QString("Width: ") + QString::number(mipDims.width));
	this->mipHeightLabel->setText(QString("Height: ") + QString::number(mipDims.height));
	this->mipDepthLabel->setText(QString("Depth: ") + QString::number(mipDims.depth));

	std::uint64_t arrayIndex = 0;  
	if (this->arraySelectorSpinBox != nullptr)
	{
		arrayIndex = this->arraySelectorSpinBox->value();
	}
	Qt::CheckState scaleMipToBaseState = this->scaleMipToBaseCheckBox->checkState();
	bool scaleMipToBase = scaleMipToBaseState == Qt::Checked ? true : false;

	updateImage(i, arrayIndex, scaleMipToBase);
}

void TexasGUI::ImageTab::mipLevelSliderChanged(int i)
{
	this->mipSelectorSpinBox->setValue(i);
}

void TexasGUI::ImageTab::scaleToMipChanged(int i)
{
	std::uint64_t arrayIndex = 0; 
	if (this->arraySelectorSpinBox != nullptr)
	{
		arrayIndex = this->arraySelectorSpinBox->value();
	}

	std::uint64_t mipLevel = this->mipSelectorSpinBox->value();

	Qt::CheckState scaleMipToBaseState = static_cast<Qt::CheckState>(i);
	bool scaleMipToBase = scaleMipToBaseState == Qt::Checked ? true : false;

	updateImage(mipLevel, arrayIndex, scaleMipToBase);
}

void TexasGUI::ImageTab::arrayLayerSpinBoxChanged(int i)
{
	std::uint64_t mipLevel = 0;
	bool scaleMipToBase = false;
	if (this->mipSelectorSpinBox != nullptr)
	{
		mipLevel = this->mipSelectorSpinBox->value();
		Qt::CheckState scaleMipToBaseState = this->scaleMipToBaseCheckBox->checkState();
		scaleMipToBase = scaleMipToBaseState == Qt::Checked ? true : false;
	}

	updateImage(mipLevel, i, scaleMipToBase);
}

void TexasGUI::ImageTab::arrayLayerSliderChanged(int i)
{
	this->arraySelectorSpinBox->setValue(i);
}

void TexasGUI::ImageTab::updateImage(std::uint64_t mipIndex, std::uint64_t arrayIndex, bool scaleMipToBase)
{
	const uchar* imgDataStart = reinterpret_cast<const uchar*>(this->imgRawData.constData());
	imgDataStart += Texas::Tools::calcArrayLayerOffset(this->imgMetaData, mipIndex, arrayIndex).value();

	const Texas::Dimensions mipDims = Texas::Tools::calcMipmapDimensions(this->imgMetaData.baseDimensions, mipIndex);

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
