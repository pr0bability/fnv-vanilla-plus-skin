#pragma once

#include "NiTexture.hpp"
#include "NiPalette.hpp"

NiSmartPointer(NiPersistentSrcTextureRendererData);

class NiPersistentSrcTextureRendererData : public NiTexture::RendererData {
public:
	NiPersistentSrcTextureRendererData();
	virtual ~NiPersistentSrcTextureRendererData();

	virtual void AllocateData(uint32_t uiLevels, uint32_t uiFaces, uint32_t uiBytes);
	virtual void FreeData();

	enum PlatformID {
		NI_ANY				= 0,
		NI_XENON			= 1,
		NI_PS3				= 2,
		NI_DX9				= 3,
		NI_NUM_PLATFORM_IDS = 4,
	};

	uint8_t*	m_pucPixels;
	uint32_t*	m_puiWidth;
	uint32_t*	m_puiHeight;
	uint32_t*	m_puiOffsetInBytes;
	uint32_t	m_uiPadOffsetInBytes;
	uint32_t	m_uiMipmapLevels;
	uint32_t	m_uiPixelStride;
	uint32_t	m_uiFaces;
	PlatformID	m_eTargetPlatform;
	uint8_t*	m_pucPristinePixels;
	uint32_t	m_uiPristineMaxOffsetInBytes;
	uint32_t	m_uiPristinePadOffsetInBytes;
	NiPalette*	m_spPalette;

	CREATE_OBJECT(NiPersistentSrcTextureRendererData, 0xA982D0);
	NIRTTI_ADDRESS(0x11F5F04);
};

ASSERT_SIZE(NiPersistentSrcTextureRendererData, 0x94)