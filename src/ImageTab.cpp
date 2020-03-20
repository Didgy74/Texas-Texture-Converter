#include "ImageTab.hpp"

#include "TexasGUI/Utilities.hpp"

#include <QBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QtMath>
#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>

#include <tuple>
#include <cstring>

#include "Texas/Tools.hpp"
#include "Texas/Texas.hpp"

namespace TexasGUI
{
	struct TexasFileStream : Texas::OutputStream
	{
		QFile file;
		QDataStream stream;

		virtual Texas::Result write(char const* data, std::uint64_t size) noexcept override
		{
			stream.writeRawData(data, static_cast<int>(size));

			return { Texas::ResultType::Success, nullptr };
		}
	};

	enum class FloatVisualizationMode
	{
		Remap,
		Clamp,
		COUNT
	};

	[[nodiscard]] QString toString(FloatVisualizationMode mode)
	{
		switch (mode)
		{
		case FloatVisualizationMode::Remap:
			return "Remap";
		case FloatVisualizationMode::Clamp:
			return "Clamp";
		default:
			return "Error";
		}
	}

	[[nodiscard]] QImage::Format toQImageFormat(Texas::PixelFormat pFormat)
	{
		switch (pFormat)
		{
		case Texas::PixelFormat::R_8:
			return QImage::Format_Grayscale8;
		case Texas::PixelFormat::RGB_8:
			return QImage::Format_RGB888;
		case Texas::PixelFormat::RGBA_8:
			return QImage::Format_RGBA8888;
		default:
			return QImage::Format_Invalid;
		}
	}

	[[nodiscard]] bool canRenderFormatNatively(Texas::PixelFormat pFormat)
	{
		return toQImageFormat(pFormat) != QImage::Format_Invalid;
	}

	[[nodiscard]] bool canDisplayFormat(Texas::PixelFormat pFormat, Texas::ChannelType chType)
	{
		if (canRenderFormatNatively(pFormat))
			return true;

		if (Texas::isCompressed(pFormat))
			return false;

		if (chType == Texas::ChannelType::SignedFloat)
			return true;

		return true;
	}

	[[nodiscard]] std::tuple<Texas::PixelFormat, Texas::ChannelType, Texas::ColorSpace> toDisplayablePixelFormat(
		Texas::PixelFormat pFormat,
		Texas::ChannelType chType,
		Texas::ColorSpace cSpace)
	{
		if (cSpace != Texas::ColorSpace::Linear && cSpace != Texas::ColorSpace::sRGB)
			return {};

		if (chType == Texas::ChannelType::SignedFloat && cSpace == Texas::ColorSpace::Linear)
		{
			switch (pFormat)
			{
			case Texas::PixelFormat::R_32:
				return { Texas::PixelFormat::R_8, chType, cSpace };
			case Texas::PixelFormat::RGB_32:
				return { Texas::PixelFormat::RGB_8, chType, cSpace };
			case Texas::PixelFormat::RGBA_32:
				return { Texas::PixelFormat::RGBA_8, chType, cSpace };
			}
		}

		return {};
	}

	[[nodiscard]] bool canFindMinMaxValues(Texas::PixelFormat pFormat)
	{
		if (Texas::isCompressed(pFormat))
			return false;
		return true;
	}

	[[nodiscard]] void testing_findMinMaxValues(Texas::Texture const& texture)
	{
		union Test
		{
			std::int64_t m_int = -1;
			std::uint64_t m_uint;
			double m_double;
		};

		Test maxValues[4] = {};
		Test minValues[4] = {};

		for (unsigned int i = 0; i < 4; i += 1)
		{
			switch (texture.channelType())
			{
			case Texas::ChannelType::SignedFloat:
				maxValues[i].m_double = std::numeric_limits<double>::min();
				minValues[i].m_double = std::numeric_limits<double>::max();
			case Texas::ChannelType::UnsignedNormalized:
				maxValues[i].m_uint = std::numeric_limits<std::uint64_t>::min();
				minValues[i].m_uint = std::numeric_limits<std::uint64_t>::max();
			}
		}

		for (std::uint64_t pixelIndex = 0; pixelIndex < texture.baseDimensions().width * texture.baseDimensions().height; pixelIndex += 1)
		{

		}
	}

	void convertFloatImageTo8Bit(
		QByteArray& byteArray, 
		QImage::Format& qImageFormat,
		Texas::Texture const& srcTexture, 
		FloatVisualizationMode visualizationMode)
	{
		Texas::PixelFormat targetFormatTemp = Texas::PixelFormat::Invalid;
		std::uint_least8_t channelCountTemp = 0;
		switch (srcTexture.pixelFormat())
		{
		case Texas::PixelFormat::R_32:
			targetFormatTemp = Texas::PixelFormat::R_8;
			channelCountTemp = 1;
			break;
		case Texas::PixelFormat::RGB_32:
			targetFormatTemp = Texas::PixelFormat::RGB_8;
			channelCountTemp = 3;
			break;
		case Texas::PixelFormat::RGBA_32:
			targetFormatTemp = Texas::PixelFormat::RGBA_8;
			channelCountTemp = 4;
			break;
		}
		Texas::PixelFormat const targetFormat = targetFormatTemp;
		std::uint_least8_t const channelCount = channelCountTemp;

		qImageFormat = toQImageFormat(targetFormat);

		auto const targetSizeRequired = Texas::calculateTotalSize(srcTexture.baseDimensions(), targetFormat, srcTexture.mipCount(), srcTexture.layerCount());

		byteArray.resize(targetSizeRequired);

		Texas::ConstByteSpan srcByteSpan = srcTexture.mipSpan(0);
		float const* const srcBuffer = reinterpret_cast<float const*>(srcByteSpan.data());
		unsigned char* const dstBuffer = reinterpret_cast<unsigned char*>(byteArray.data());

		std::size_t const pixelCount = srcTexture.baseDimensions().width * srcTexture.baseDimensions().height;

		if (visualizationMode == FloatVisualizationMode::Remap)
		{
			float maxValue[4] = { std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min() };
			float minValue[4] = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
			// Loop over the base mip level and find each channels max values
			for (std::size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex += 1)
			{
				for (size_t channelIndex = 0; channelIndex < channelCount; channelIndex += 1)
				{
					if (srcBuffer[pixelIndex * channelCount + channelIndex] > maxValue[channelIndex])
						maxValue[channelIndex] = srcBuffer[pixelIndex * channelCount + channelIndex];
					else if (srcBuffer[pixelIndex * channelCount + channelIndex] < minValue[channelIndex])
						minValue[channelIndex] = srcBuffer[pixelIndex * channelCount + channelIndex];
				}
			}

			// Apply the remapping
			for (std::size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex += 1)
			{
				for (size_t channelIndex = 0; channelIndex < channelCount; channelIndex += 1)
				{
					float srcValue = srcBuffer[pixelIndex * channelCount + channelIndex];

					dstBuffer[pixelIndex * channelCount + channelIndex] = static_cast<unsigned char>((srcValue - minValue[channelIndex]) / (maxValue[channelIndex] - minValue[channelIndex]) * 255);
				}
			}
		}
		else if (visualizationMode == FloatVisualizationMode::Clamp)
		{
			for (std::size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex += 1)
			{
				for (size_t channelIndex = 0; channelIndex < channelCount; channelIndex += 1)
				{
					float value = srcBuffer[pixelIndex * channelCount + channelIndex];
					if (value > 1.f)
						value = 1.f;
					else if (value < 0.f)
						value = 0.f;
					dstBuffer[pixelIndex * channelCount + channelIndex] = static_cast<unsigned char>(value * 255);
				}
			}
		}
	}

	void convertRA8_To_R8(
		QByteArray& byteArray,
		QImage::Format& qImageFormat,
		Texas::Texture const& srcTexture)
	{
		qImageFormat = QImage::Format_Grayscale8;

		std::uint64_t customImgDataSize = Texas::calculateTotalSize(
			srcTexture.baseDimensions(), 
			Texas::PixelFormat::R_8, 
			1, 
			1);
		byteArray.resize((int)customImgDataSize);

		std::uint64_t const srcRowWidth = srcTexture.baseDimensions().width * 2;
		std::uint64_t const dstRowWidth = srcTexture.baseDimensions().width;
		for (std::uint64_t y = 0; y < srcTexture.baseDimensions().height; y += 1)
		{
			std::byte const* srcRowStart = srcTexture.layerSpan(0,0).data() + srcRowWidth * y;
			std::byte* dstRowStart = (std::byte*)byteArray.data() + dstRowWidth * y;
			for (std::uint64_t x = 0; x < srcTexture.baseDimensions().width; x += 1)
			{
				std::memcpy(dstRowStart + x, srcRowStart + (x * 2), 1);
			}
		}
	}
}

TexasGUI::ImageTab::ImageTab(
	QString const& fullPath,
	Texas::Texture&& texture) :
	QWidget(),
	sourceTexture(static_cast<Texas::Texture&&>(texture))
{
	QHBoxLayout* outerLayout = new QHBoxLayout;
	this->setLayout(outerLayout);

	// Figure out if we can display the texture
	bool canDisplay = canDisplayFormat(this->sourceTexture.pixelFormat(), this->sourceTexture.channelType());

	this->createLeftPanel(outerLayout, fullPath, canDisplay);

	QScrollArea* imageScrollArea = new QScrollArea;
	outerLayout->addWidget(imageScrollArea);

	this->imgLabel = new QLabel;
	imageScrollArea->setWidget(this->imgLabel);

	if (canDisplay)
	{
		// Figure out if we can render the source data
		if (canRenderFormatNatively(this->sourceTexture.pixelFormat()))
		{
			// We can render the source data
			
		}
		else
		{
			// We have to convert the source data to a format we can render.


			// TODO: Convert RA to just R
			/*
			if (this->sourceTexture.pixelFormat() == Texas::PixelFormat::RA_8)
			{
				convertRA8_To_R8(this->customImgData, this->qImgFormat, this->sourceTexture);
			}
			*/
			if (this->sourceTexture.channelType() == Texas::ChannelType::SignedFloat)
			{
				convertFloatImageTo8Bit(this->customImgData, this->qImgFormat, this->sourceTexture, FloatVisualizationMode(0));
			}
		}

		updateImage(0, 0, false);
	}
	else
	{
		this->imgLabel->setText("Unable to display this texture.");
		this->imgLabel->adjustSize();
	}
}

void TexasGUI::ImageTab::createLeftPanel(QLayout* parentLayout, QString const& fullPath, bool enableControls)
{
	QWidget* controlsWidget = new QWidget;
	parentLayout->addWidget(controlsWidget);
	controlsWidget->setMinimumWidth(250);
	controlsWidget->setMaximumWidth(250);

	QVBoxLayout* outerVLayout = new QVBoxLayout;
	controlsWidget->setLayout(outerVLayout);
	outerVLayout->setMargin(0);

	if (enableControls)
	{
		if (this->sourceTexture.channelType() == Texas::ChannelType::SignedFloat)
			createFloatVisualizationControls(outerVLayout);

		if (this->sourceTexture.mipCount() > 1)
			createMipControls(outerVLayout);

		if (this->sourceTexture.layerCount() > 1)
			createArrayControls(outerVLayout);
	}

	createDetailsBox(outerVLayout);


	QPushButton* button = new QPushButton;
	outerVLayout->addWidget(button);
	button->setText("Export to KTX");
	button->setEnabled(false);

	Texas::Result canSaveTextureAsKTX = Texas::KTX::canSave(this->sourceTexture.textureInfo());
	if (Texas::KTX::canSave(this->sourceTexture.textureInfo()).isSuccessful())
	{
		button->setEnabled(true);
		// Connect button
		QObject::connect(button, SIGNAL(clicked()), this, SLOT(exportAsKTX()));
	}




	QSpacerItem* endOfControlsSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	outerVLayout->addSpacerItem(endOfControlsSpacer);
}

void TexasGUI::ImageTab::createFloatVisualizationControls(QLayout* parentLayout)
{
	QGroupBox* box = new QGroupBox;
	parentLayout->addWidget(box);
	box->setTitle("Float visualization");

	QVBoxLayout* innerVLayout = new QVBoxLayout;
	box->setLayout(innerVLayout);

	QComboBox* modeDropdown = new QComboBox;
	innerVLayout->addWidget(modeDropdown);

	// Add the visualization options, in the order they are defined
	for (std::size_t i = 0; i < static_cast<std::size_t>(FloatVisualizationMode::COUNT); i += 1)
		modeDropdown->addItem(toString(static_cast<FloatVisualizationMode>(i)));

	QObject::connect(modeDropdown, SIGNAL(currentIndexChanged(int)), this, SLOT(floatVisualizationModeChanged(int)));

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
		this->mipSelectorSpinBox->setMaximum(this->sourceTexture.mipCount() - 1);
		QObject::connect(this->mipSelectorSpinBox, SIGNAL(valueChanged(int)), this, SLOT(mipLevelSpinBoxChanged(int)));

		QLabel* maxMipTextLabel = new QLabel;
		levelSelectorLayout->addWidget(maxMipTextLabel);
		maxMipTextLabel->setText(QString("/ ") + QString::number(this->sourceTexture.mipCount() - 1));
	}

	// Make the mip slider
	this->mipSelectorSlider = new QSlider;
	innerVLayout->addWidget(this->mipSelectorSlider);
	this->mipSelectorSlider->setMaximum(this->sourceTexture.mipCount() - 1);
	this->mipSelectorSlider->setPageStep(1);
	this->mipSelectorSlider->setOrientation(Qt::Horizontal);
	this->mipSelectorSlider->setTickPosition(QSlider::TicksBelow);
	QObject::connect(this->mipSelectorSlider, SIGNAL(valueChanged(int)), this, SLOT(mipLevelSliderChanged(int)));

	Texas::Dimensions mipDims = Texas::calculateMipDimensions(this->sourceTexture.baseDimensions(), 0);

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
		this->arraySelectorSpinBox->setMaximum(this->sourceTexture.layerCount() - 1);
		QObject::connect(this->arraySelectorSpinBox, SIGNAL(valueChanged(int)), this, SLOT(arrayLayerSpinBoxChanged(int)));

		QLabel* maxMipTextLabel = new QLabel;
		levelSelectorLayout->addWidget(maxMipTextLabel);
		maxMipTextLabel->setText(QString("/ ") + QString::number(this->sourceTexture.layerCount() - 1));
	}

	this->arraySelectorSlider = new QSlider;
	innerVLayout->addWidget(this->arraySelectorSlider);
	this->arraySelectorSlider->setMaximum(this->sourceTexture.layerCount() - 1);
	this->arraySelectorSlider->setPageStep(1);
	this->arraySelectorSlider->setOrientation(Qt::Horizontal);
	this->arraySelectorSlider->setTickPosition(QSlider::TicksBelow);
	QObject::connect(this->arraySelectorSlider, SIGNAL(valueChanged(int)), this, SLOT(arrayLayerSliderChanged(int)));
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
	textureTypeLabel->setText("Type: " + TexasGUI::Utils::toString(this->sourceTexture.textureType()));

	QLabel* formatLabel = new QLabel;
	vLayout->addWidget(formatLabel);
	formatLabel->setText("Format: " + TexasGUI::Utils::toString(this->sourceTexture.pixelFormat()));

	QLabel* channelTypeLabel = new QLabel;
	vLayout->addWidget(channelTypeLabel);
	channelTypeLabel->setText("Channel type: " + TexasGUI::Utils::toString(this->sourceTexture.channelType()));

	QLabel* colorSpaceLabel = new QLabel;
	vLayout->addWidget(colorSpaceLabel);
	colorSpaceLabel->setText("Color space: " + TexasGUI::Utils::toString(this->sourceTexture.colorSpace()));

	QLabel* widthLabel = new QLabel;
	vLayout->addWidget(widthLabel);
	widthLabel->setText("Width: " + QString::number(this->sourceTexture.baseDimensions().width));

	QLabel* heightLabel = new QLabel;
	vLayout->addWidget(heightLabel);
	heightLabel->setText("Height: " + QString::number(this->sourceTexture.baseDimensions().height));

	QLabel* depthLabel = new QLabel;
	vLayout->addWidget(depthLabel);
	depthLabel->setText("Depth: " + QString::number(this->sourceTexture.baseDimensions().depth));

	QLabel* mipCountLabel = new QLabel;
	vLayout->addWidget(mipCountLabel);
	mipCountLabel->setText("Mip levels: " + QString::number(this->sourceTexture.mipCount()));

	QLabel* arrayLayerLabel = new QLabel;
	vLayout->addWidget(arrayLayerLabel);
	arrayLayerLabel->setText("Array layers: " + QString::number(this->sourceTexture.layerCount()));

	QLabel* srcFileFormatLabel = new QLabel;
	vLayout->addWidget(srcFileFormatLabel);
	srcFileFormatLabel->setText("File-format: " + TexasGUI::Utils::toString(this->sourceTexture.fileFormat()));
}

void TexasGUI::ImageTab::floatVisualizationModeChanged(int i)
{
	FloatVisualizationMode newMode = static_cast<FloatVisualizationMode>(i);

	convertFloatImageTo8Bit(this->customImgData, this->qImgFormat, this->sourceTexture, newMode);

	unsigned int mipLevel = getCurrentMipLevel();
	bool scaleMipToBase = getScaleMipToBase();
	unsigned int arrayIndex = getCurrentArrayLayer();

	updateImage(mipLevel, arrayIndex, scaleMipToBase);
}

void TexasGUI::ImageTab::mipLevelSpinBoxChanged(int i)
{
	this->mipSelectorSlider->setValue(i);

	const Texas::Dimensions mipDims = Texas::calculateMipDimensions(this->sourceTexture.baseDimensions(), i);

	this->mipWidthLabel->setText(QString("Width: ") + QString::number(mipDims.width));
	this->mipHeightLabel->setText(QString("Height: ") + QString::number(mipDims.height));
	this->mipDepthLabel->setText(QString("Depth: ") + QString::number(mipDims.depth));

	unsigned int arrayIndex = getCurrentArrayLayer();
	bool scaleMipToBase = getScaleMipToBase();

	updateImage(i, arrayIndex, scaleMipToBase);
}

void TexasGUI::ImageTab::mipLevelSliderChanged(int i)
{
	this->mipSelectorSpinBox->setValue(i);
}

void TexasGUI::ImageTab::scaleToMipChanged(int i)
{
	unsigned int arrayIndex = getCurrentArrayLayer();

	unsigned int mipLevel = getCurrentMipLevel();

	Qt::CheckState scaleMipToBaseState = static_cast<Qt::CheckState>(i);
	bool scaleMipToBase = scaleMipToBaseState == Qt::Checked ? true : false;

	updateImage(mipLevel, arrayIndex, scaleMipToBase);
}

void TexasGUI::ImageTab::arrayLayerSpinBoxChanged(int i)
{
	unsigned int mipLevel = getCurrentMipLevel();
	bool scaleMipToBase = getScaleMipToBase();

	updateImage(mipLevel, i, scaleMipToBase);
}

void TexasGUI::ImageTab::arrayLayerSliderChanged(int i)
{
	this->arraySelectorSpinBox->setValue(i);
}

void TexasGUI::ImageTab::exportAsKTX()
{
	QString fileName = QFileDialog::getSaveFileName(this, "Save file as KTX", "", "KTX Image (*.ktx)");
	if (!fileName.isEmpty())
	{
		TexasFileStream fileStream;
		fileStream.file.setFileName(fileName);
		fileStream.file.open(QIODevice::OpenModeFlag::WriteOnly);
		if (fileStream.file.isOpen())
		{
			fileStream.stream.setDevice(&fileStream.file);
			fileStream.stream.setByteOrder(QDataStream::LittleEndian);

			Texas::Result writeResult = Texas::KTX::saveToStream(this->sourceTexture, fileStream);
			if (!writeResult.isSuccessful())
			{
				// Do some error handling.
				Utils::displayErrorBox("Unable to save file.", writeResult.errorMessage());
			}
		}
	}
}

unsigned int TexasGUI::ImageTab::getCurrentMipLevel() const
{
	unsigned int mipLevel = 0;
	if (this->mipSelectorSpinBox != nullptr)
		mipLevel = this->mipSelectorSpinBox->value();
	return mipLevel;
}

bool TexasGUI::ImageTab::getScaleMipToBase() const
{
	bool scaleMipToBase = false;
	if (this->mipSelectorSpinBox != nullptr)
		scaleMipToBase = this->scaleMipToBaseCheckBox->checkState() == Qt::Checked ? true : false;
	return scaleMipToBase;
}

unsigned int TexasGUI::ImageTab::getCurrentArrayLayer() const
{
	unsigned int arrayIndex = 0;
	if (this->arraySelectorSpinBox != nullptr)
		arrayIndex = this->arraySelectorSpinBox->value();
	return arrayIndex;
}

void TexasGUI::ImageTab::updateImage(std::uint64_t mipIndex, std::uint64_t arrayIndex, bool scaleMipToBase)
{
	QImage imageToDisplay{};

	Texas::Dimensions const mipDims = Texas::calculateMipDimensions(this->sourceTexture.baseDimensions(), mipIndex);

	if (canRenderFormatNatively(this->sourceTexture.pixelFormat()))
	{
		uchar const* imgData = reinterpret_cast<uchar const*>(this->sourceTexture.layerSpan(mipIndex, arrayIndex).data());

		imageToDisplay = QImage(imgData, mipDims.width, mipDims.height, toQImageFormat(this->sourceTexture.pixelFormat()));
	}
	else
	{
		uchar const* imgData = reinterpret_cast<uchar const*>(this->customImgData.data());

		imageToDisplay = QImage(imgData, mipDims.width, mipDims.height, this->qImgFormat);
	}

	QPixmap tempPixMap = QPixmap::fromImage(imageToDisplay);

	if (scaleMipToBase == false || mipIndex == 0)
	{
		
	}
	else
	{
		// Scale the image
		QSize size = qPow(2, mipIndex) * tempPixMap.size();
		tempPixMap = tempPixMap.scaled(size, Qt::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
	}

	this->imgLabel->setPixmap(tempPixMap);
	this->imgLabel->adjustSize();
}
