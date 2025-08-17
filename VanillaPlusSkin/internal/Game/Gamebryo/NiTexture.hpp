#pragma once

#include "NiObjectNET.hpp"
#include "NiPixelFormat.hpp"

NiSmartPointer(NiTexture);

class NiFixedString;
class NiDX9TextureData;

class NiTexture : public NiObjectNET {
public:
	NiTexture();
	virtual ~NiTexture();

	virtual void			Upgrade();
	virtual void			Degrade();
	virtual uint32_t		GetWidth() const;
	virtual uint32_t		GetHeight() const;
	virtual NiFixedString*	GetPath() const;
	virtual uint32_t		GetSize() const;

	class FormatPrefs {
	public:
		FormatPrefs() : m_ePixelLayout(TRUE_COLOR_32), m_eAlphaFmt(SMOOTH), m_eMipMapped(YES) {};
		~FormatPrefs() {};

		enum PixelLayout {
			PALETTIZED_8	= 0,
			HIGH_COLOR_16	= 1,
			TRUE_COLOR_32	= 2,
			COMPRESSED		= 3,
			BUMPMAP			= 4,
			PALETTIZED_4	= 5,
			PIX_DEFAULT		= 6,
			SINGLE_COLOR_8	= 7,
			SINGLE_COLOR_16 = 8,
			SINGLE_COLOR_32 = 9,
			DOUBLE_COLOR_32 = 10,
			DOUBLE_COLOR_64 = 11,
			FLOAT_COLOR_32	= 12,
			FLOAT_COLOR_64	= 13,
			FLOAT_COLOR_128 = 14,
		};

		enum AlphaFormat {
			BINARY			= 1,
			SMOOTH			= 2,
			ALPHA_DEFAULT	= 3,
		};

		enum MipFlag {
			NO			= 0,
			YES			= 1,
			MIP_DEFAULT = 2,
		};

		FormatPrefs(const PixelLayout& m_ePixelLayout, const AlphaFormat& m_eAlphaFmt, const MipFlag& m_eMipMapped)
			: m_ePixelLayout(m_ePixelLayout), m_eAlphaFmt(m_eAlphaFmt), m_eMipMapped(m_eMipMapped) {}

		PixelLayout		m_ePixelLayout;
		AlphaFormat		m_eAlphaFmt;
		MipFlag			m_eMipMapped;
	};

	class RendererData : public NiObject {
	public:
		RendererData();
		virtual ~RendererData();

		virtual uint32_t	UpgradeTexture(NiTexture* apTexture);
		virtual uint32_t	DegradeTexture(NiTexture* apTexture);
		virtual void		Unk_03();

		enum State : uint8_t {
			NONE		= 0,
			DEGRADING	= 1,
			UPGRADING	= 2,
			DEGRADED	= 3,
			UPGRADED	= 4,
		};

		struct StreamingData {
			char			unk58;
			char			unk59;
			char			unk5A;
			State			ucState;
		};

		NiTexture*		m_pkTexture;
		uint32_t		m_uiWidth;
		uint32_t		m_uiHeight;
		NiPixelFormat	m_kPixelFormat;
		StreamingData   kStreamingData;
		Bitfield8		ucFlags;

		NiNewRTTI(NiTexture::RendererData, NiObject);

		bool IsAlphaTexture() const {
			return m_kPixelFormat.IsAlpha();
		}
	};

	FormatPrefs		m_kFormatPrefs;
	RendererData*	m_pkRendererData;
	NiTexture*		m_pkPrev;
	NiTexture*		m_pkNext;

	RendererData* GetRendererData() const {
		return m_pkRendererData;
	}

	NiDX9TextureData* GetDX9RendererData() const {
		return reinterpret_cast<NiDX9TextureData*>(m_pkRendererData);
	};

	bool IsAlphaTexture() const {
		return m_pkRendererData->IsAlphaTexture();
	}
};

ASSERT_SIZE(NiTexture, 0x30);