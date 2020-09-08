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

	template<Texas::PixelFormat pixelFormat, Texas::ChannelType channelType>
	void FindMinMaxValues_Internal(
		Texas::TextureInfo texInfo,
		Texas::ConstByteSpan byteSpan,
		MinMaxData& minMaxData) = delete;

	template<Texas::PixelFormat pixelFormat, Texas::ChannelType channelType>
	void BuildDisplayableTexture_Internal(
		Texas::TextureInfo texInfo,
		Texas::ConstByteSpan byteSpan,
		QByteArray& byteArray) = delete;

	template<>
	void FindMinMaxValues_Internal<Texas::PixelFormat::RGB_8, Texas::ChannelType::UnsignedNormalized>(
		Texas::TextureInfo texInfo,
		Texas::ConstByteSpan byteSpan,
		MinMaxData& minMaxData)
	{
		minMaxData.mipLevels.resize(texInfo.mipCount);
		for (auto& mipLevel : minMaxData.mipLevels)
			mipLevel.layers.resize(texInfo.layerCount);
		for (uint8_t mipIndex = 0; mipIndex < texInfo.mipCount; mipIndex++)
		{
			auto& mipLevel = minMaxData.mipLevels[mipIndex];

			Texas::Dimensions mipDimensions = Texas::calculateMipDimensions(texInfo.baseDimensions, mipIndex);
			uint64_t pixelCount = mipDimensions.width * mipDimensions.height * mipDimensions.depth;
			uint64_t mipMemoryOffset = Texas::calculateMipOffset(texInfo, mipIndex);

			for (uint8_t layerIndex = 0; layerIndex < texInfo.layerCount; layerIndex++)
			{
				auto& layer = mipLevel.layers[layerIndex];
				uint64_t layerMemoryOffset = Texas::calculateLayerOffset(texInfo, mipIndex, layerIndex);

				for (uint8_t i = 0; i < 3; i++)
				{
					layer.min_uint64[i] = std::numeric_limits<uint64_t>::max();
					layer.max_uint64[i] = std::numeric_limits<uint64_t>::min();
				}
				layer.min_uint64[3] = 0;
				layer.max_uint64[3] = 0;

				for (uint64_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++)
				{
					unsigned char const* srcPixel = (unsigned char const*)byteSpan.data() + mipMemoryOffset + layerMemoryOffset + pixelIndex * 3;
					for (size_t i = 0; i < 3; i++)
					{
						layer.min_uint64[i] = std::min(layer.min_uint64[i], (uint64_t)srcPixel[i]);
						layer.max_uint64[i] = std::max(layer.min_uint64[i], (uint64_t)srcPixel[i]);
					}
				}
			}
		}
	}

	template<>
	void BuildDisplayableTexture_Internal<Texas::PixelFormat::RGB_8, Texas::ChannelType::UnsignedNormalized>(
		Texas::TextureInfo texInfo,
		Texas::ConstByteSpan byteSpan, 
		QByteArray& byteArray)
	{
		Texas::TextureInfo dstTexInfo = texInfo;
		dstTexInfo.pixelFormat = Texas::PixelFormat::RGBA_8;
		dstTexInfo.channelType = Texas::ChannelType::UnsignedNormalized;
		byteArray = QByteArray(Texas::calculateTotalSize(dstTexInfo), Qt::Initialization::Uninitialized);
		
		size_t linearLength = texInfo.baseDimensions.width * texInfo.baseDimensions.height;
		// Copy the three first channels
		for (size_t pixelIndex = 0; pixelIndex < linearLength; pixelIndex++)
		{
			unsigned char const* srcPixel = (unsigned char const*)byteSpan.data() + pixelIndex * 3;
			unsigned char* dstPixel = (unsigned char*)byteArray.data() + pixelIndex * 4;
			
			dstPixel[0] = srcPixel[0];
			dstPixel[1] = srcPixel[1];
			dstPixel[2] = srcPixel[2];
			dstPixel[3] = 255;
		}
	}

	template<>
	void FindMinMaxValues_Internal<Texas::PixelFormat::RGBA_8, Texas::ChannelType::UnsignedNormalized>(
		Texas::TextureInfo texInfo,
		Texas::ConstByteSpan byteSpan,
		MinMaxData& minMaxData)
	{
		minMaxData.mipLevels.resize(texInfo.mipCount);
		for (auto& mipLevel : minMaxData.mipLevels)
			mipLevel.layers.resize(texInfo.layerCount);
		for (uint8_t mipIndex = 0; mipIndex < texInfo.mipCount; mipIndex++)
		{
			auto& mipLevel = minMaxData.mipLevels[mipIndex];

			Texas::Dimensions mipDimensions = Texas::calculateMipDimensions(texInfo.baseDimensions, mipIndex);
			uint64_t pixelCount = mipDimensions.width * mipDimensions.height * mipDimensions.depth;

			for (uint8_t layerIndex = 0; layerIndex < texInfo.layerCount; layerIndex++)
			{
				auto& layer = mipLevel.layers[layerIndex];
				uint64_t layerMemoryOffset = Texas::calculateLayerOffset(texInfo, mipIndex, layerIndex);

				for (uint8_t i = 0; i < 4; i++)
				{
					layer.min_uint64[i] = std::numeric_limits<uint64_t>::max();
					layer.max_uint64[i] = std::numeric_limits<uint64_t>::min();
				}

				for (uint64_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++)
				{
					unsigned char const* srcPixel = (unsigned char const*)byteSpan.data() + layerMemoryOffset + pixelIndex * 4;
					for (size_t i = 0; i < 4; i++)
					{
						layer.min_uint64[i] = std::min(layer.min_uint64[i], (uint64_t)srcPixel[i]);
						layer.max_uint64[i] = std::max(layer.max_uint64[i], (uint64_t)srcPixel[i]);
					}
				}
			}
		}
	}

	template<>
	void BuildDisplayableTexture_Internal<Texas::PixelFormat::RGBA_8, Texas::ChannelType::UnsignedNormalized>(
		Texas::TextureInfo texInfo,
		Texas::ConstByteSpan byteSpan,
		QByteArray& byteArray)
	{
		Texas::TextureInfo dstTexInfo = texInfo;
		dstTexInfo.pixelFormat = Texas::PixelFormat::RGBA_8;
		dstTexInfo.channelType = Texas::ChannelType::UnsignedNormalized;
		byteArray = QByteArray(Texas::calculateTotalSize(dstTexInfo), Qt::Initialization::Uninitialized);

		for (uint8_t mipIndex = 0; mipIndex < texInfo.mipCount; mipIndex++)
		{
			Texas::Dimensions mipDimensions = Texas::calculateMipDimensions(texInfo.baseDimensions, mipIndex);
			uint64_t pixelCount = mipDimensions.width * mipDimensions.height * mipDimensions.depth;

			for (uint8_t layerIndex = 0; layerIndex < texInfo.layerCount; layerIndex++)
			{
				uint64_t srcLayerMemoryOffset = Texas::calculateLayerOffset(texInfo, mipIndex, layerIndex);
				uint64_t dstLayerMemoryOffset = Texas::calculateLayerOffset(dstTexInfo, mipIndex, layerIndex);
				for (size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++)
				{
					unsigned char const* srcPixel = (unsigned char const*)byteSpan.data() + srcLayerMemoryOffset + pixelIndex * 4;
					unsigned char* dstPixel = (unsigned char*)byteArray.data() + dstLayerMemoryOffset + pixelIndex * 4;

					dstPixel[0] = srcPixel[0];
					dstPixel[1] = srcPixel[1];
					dstPixel[2] = srcPixel[2];
					dstPixel[3] = srcPixel[3];
				}
			}
		}
	}

	void FindMinMaxValues(
		Texas::TextureInfo texInfo,
		Texas::ConstByteSpan byteSpan,
		MinMaxData& minMaxData)
	{
		switch (texInfo.pixelFormat)
		{
		case Texas::PixelFormat::RGB_8:
		{
			switch (texInfo.channelType)
			{
			case Texas::ChannelType::UnsignedNormalized:
				FindMinMaxValues_Internal<Texas::PixelFormat::RGB_8, Texas::ChannelType::UnsignedNormalized>(
					texInfo,
					byteSpan,
					minMaxData);
			}
			break;
		}
		case Texas::PixelFormat::RGBA_8:
		{
			switch (texInfo.channelType)
			{
			case Texas::ChannelType::UnsignedNormalized:
				FindMinMaxValues_Internal<Texas::PixelFormat::RGBA_8, Texas::ChannelType::UnsignedNormalized>(
					texInfo,
					byteSpan,
					minMaxData);
			}
			break;
		}
		break;
		}
	}

	void BuildDisplayableTexture(
		Texas::TextureInfo texInfo,
		Texas::ConstByteSpan byteSpan,
		QByteArray& byteArray)
	{
		switch (texInfo.pixelFormat)
		{
		case Texas::PixelFormat::RGB_8:
		{
			switch (texInfo.channelType)
			{
			case Texas::ChannelType::UnsignedNormalized:
				BuildDisplayableTexture_Internal<Texas::PixelFormat::RGB_8, Texas::ChannelType::UnsignedNormalized>(
					texInfo,
					byteSpan,
					byteArray);
			}
			break;
		}
		case Texas::PixelFormat::RGBA_8:
		{
			switch (texInfo.channelType)
			{
			case Texas::ChannelType::UnsignedNormalized:
				BuildDisplayableTexture_Internal<Texas::PixelFormat::RGBA_8, Texas::ChannelType::UnsignedNormalized>(
					texInfo,
					byteSpan,
					byteArray);
			}
			break;
		}
		break;
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

	FindMinMaxValues(
		this->sourceTexture.textureInfo(),
		this->sourceTexture.rawBufferSpan(),
		this->minMaxData);
	BuildDisplayableTexture(
		this->sourceTexture.textureInfo(),
		this->sourceTexture.rawBufferSpan(),
		this->customImgData);

	this->createLeftPanel(outerLayout, fullPath, true);

	QScrollArea* imageScrollArea = new QScrollArea;
	outerLayout->addWidget(imageScrollArea);

	this->imgLabel = new QLabel;
	imageScrollArea->setWidget(this->imgLabel);

	if (true)
	{
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

		createMinMaxBox(outerVLayout);
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

void TexasGUI::ImageTab::createMinMaxBox(QLayout* parentLayout)
{
	QGroupBox* box = new QGroupBox;
	parentLayout->addWidget(box);
	box->setTitle("Min/Max");

	QVBoxLayout* innerLayout = new QVBoxLayout;
	box->setLayout(innerLayout);
	for (uint8_t i = 0; i < 4; i++)
	{
		this->minMaxLabels.min[i] = new QLabel;
		innerLayout->addWidget(this->minMaxLabels.min[i]);
	}
	for (uint8_t i = 0; i < 4; i++)
	{
		this->minMaxLabels.max[i] = new QLabel;
		innerLayout->addWidget(this->minMaxLabels.max[i]);
	}
	updateMinMaxLabels(0, 0);
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
	updateMinMaxLabels(i, arrayIndex);
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
	updateMinMaxLabels(mipLevel, arrayIndex);
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

void TexasGUI::ImageTab::updateMinMaxLabels(uint8_t mipIndex, uint64_t layerIndex)
{
	auto& layer = this->minMaxData.mipLevels[mipIndex].layers[layerIndex];

	for (uint8_t i = 0; i < 4; i++)
	{
		std::string text = std::to_string(i) + " " + "Min: " + std::to_string(layer.min_uint64[i]);
		this->minMaxLabels.min[i]->setText(QString::fromStdString(text));
		text = std::to_string(i) + " " + "Max: " + std::to_string(layer.max_uint64[i]);
		this->minMaxLabels.max[i]->setText(QString::fromStdString(text));
	}

}

void TexasGUI::ImageTab::updateImage(std::uint8_t mipIndex, std::uint64_t arrayIndex, bool scaleMipToBase)
{
	QImage imageToDisplay{};

	Texas::Dimensions const mipDims = Texas::calculateMipDimensions(this->sourceTexture.baseDimensions(), mipIndex);

	QPixmap tempPixMap = QPixmap::fromImage(imageToDisplay);

	uint64_t imgDataMemoryOffset = Texas::calculateLayerOffset(
		this->sourceTexture.baseDimensions(), 
		Texas::PixelFormat::RGBA_8, mipIndex,
		this->sourceTexture.layerCount(), 
		arrayIndex);
	uchar const* imgData = (uchar const*)this->customImgData.constData() + imgDataMemoryOffset;
	imageToDisplay = QImage(imgData, mipDims.width, mipDims.height, QImage::Format::Format_RGBA8888);
	tempPixMap = QPixmap::fromImage(imageToDisplay);

	if (scaleMipToBase && mipIndex != 0)
	{
		// Scale the image
		QSize size = qPow(2, mipIndex) * tempPixMap.size();
		//tempPixMap = tempPixMap.scaled(size, Qt::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
		tempPixMap = tempPixMap.scaled(size, Qt::IgnoreAspectRatio, Qt::TransformationMode::FastTransformation);
	}

	this->imgLabel->setPixmap(tempPixMap);
	this->imgLabel->adjustSize();
}
