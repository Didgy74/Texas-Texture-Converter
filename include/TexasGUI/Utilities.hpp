#pragma once

#include <QString>
#include <QMessageBox>

#include "Texas/ResultType.hpp"

namespace TexasGUI::Utils
{
    [[nodiscard]] inline QString toErrorBoxText(Texas::ResultType result)
    {
        switch (result)
        {
        case Texas::ResultType::CorruptFileData:
            return "This image file is corrupt.";
        case Texas::ResultType::FileNotSupported:
            return "Texas does not support this file.";
        default:
            assert(true && "Hit an unhandled case in TexasGUI::toErrorBoxText");
            return "";
        }
    }

    inline void displayErrorBox(QString const& title, QString const& detailedText)
    {
        assert(title.size() > 0 && "Tried running TexasGUI::displayErrorBox without any title.");
        QMessageBox msgBox;
        msgBox.setText(title);
        if (detailedText.size() > 0)
            msgBox.setInformativeText(detailedText);
        msgBox.exec();
    }

	[[nodiscard]] inline QString toString(Texas::TextureType in)
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

	[[nodiscard]] inline QString toString(Texas::PixelFormat in)
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
		case PixelFormat::BGR_8:
			return "BGR_8";
		case PixelFormat::RGBA_8:
			return "RGBA_8";
		case PixelFormat::BGRA_8:
			return "BGRA_8";

		case PixelFormat::R_16:
			return "R_16";
		case PixelFormat::RG_16:
			return "RG_16";
		case PixelFormat::RA_16:
			return "RA_16";
		case PixelFormat::RGB_16:
			return "RGB_16";
		case PixelFormat::BGR_16:
			return "BGR_16";
		case PixelFormat::RGBA_16:
			return "RGBA_16";
		case PixelFormat::BGRA_16:
			return "BGRA_16";

		case PixelFormat::R_32:
			return "R_32";
		case PixelFormat::RG_32:
			return "RG_32";
		case PixelFormat::RA_32:
			return "RA_32";
		case PixelFormat::RGB_32:
			return "RGB_32";
		case PixelFormat::BGR_32:
			return "BGR_32";
		case PixelFormat::RGBA_32:
			return "RGBA_32";
		case PixelFormat::BGRA_32:
			return "BGRA_32";

		case PixelFormat::BC1_RGB:
			return "BC1_RGB";
		case PixelFormat::BC1_RGBA:
			return "BC1_RGBA";
		case PixelFormat::BC2_RGBA:
			return "BC2_RGBA";
		case PixelFormat::BC3_RGBA:
			return "BC3_RGBA";
		case PixelFormat::BC4:
			return "BC4";
		case PixelFormat::BC5:
			return "BC5";
		case PixelFormat::BC6H:
			return "BC6H";
		case PixelFormat::BC7_RGBA:
			return "BC7_RGBA";

		default:
			return "Error";
		}
	}

	[[nodiscard]] inline QString toString(Texas::ChannelType in)
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

	[[nodiscard]] inline QString toString(Texas::ColorSpace in)
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

	[[nodiscard]] inline QString toString(Texas::FileFormat in)
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

	[[nodiscard]] inline std::uint_least8_t channelCount(Texas::PixelFormat pFormat)
	{
		switch (pFormat)
		{
		case Texas::PixelFormat::R_8:
		case Texas::PixelFormat::R_16:
		case Texas::PixelFormat::R_32:
			return 1;

		case Texas::PixelFormat::RA_8:
		case Texas::PixelFormat::RG_8:
		case Texas::PixelFormat::RGB_8:
		case Texas::PixelFormat::BGR_8:
		case Texas::PixelFormat::RGBA_8:
		case Texas::PixelFormat::BGRA_8:

		
		case Texas::PixelFormat::RA_16:
		case Texas::PixelFormat::RG_16:
		case Texas::PixelFormat::RGB_16:
		case Texas::PixelFormat::BGR_16:
		case Texas::PixelFormat::RGBA_16:
		case Texas::PixelFormat::BGRA_16:

		
		case Texas::PixelFormat::RG_32:
		case Texas::PixelFormat::RA_32:
		case Texas::PixelFormat::RGB_32:
		case Texas::PixelFormat::BGR_32:
		case Texas::PixelFormat::RGBA_32:
		case Texas::PixelFormat::BGRA_32:
			return 0;
		}

		return 0;
	}
}