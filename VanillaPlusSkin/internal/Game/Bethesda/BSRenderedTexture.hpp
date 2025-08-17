#pragma once

#include "Gamebryo/NiRenderedTexture.hpp"
#include "Gamebryo/NiRenderTargetGroup.hpp"
#include "Gamebryo/NiRenderedCubeMap.hpp"
#include "BSString.hpp"
#include "Gamebryo/NiDX9TextureData.hpp"

NiSmartPointer(BSRenderedTexture);

class BSRenderedTexture : public NiObject {
public:
	NiRenderTargetGroupPtr	spRenderTargetGroups[6];
	NiRenderTargetGroupPtr	spPreRestoreGroup;
	NiObjectPtr				spObject024;
	NiObjectPtr				spObject028;
	int32_t					eType; // BSTextureManager::eTextureType
	NiRenderedTexturePtr	spRenderedTextures[4];

	NiNewRTTI(BSRenderedTexture, NiObject);

	static BSRenderedTexture* Create(const NiFixedString& arName, const uint32_t uiWidth, const uint32_t uiHeight,
		NiTexture::FormatPrefs* kPrefs, Ni2DBuffer::MultiSamplePreference eMSAAPref,
		bool bUseDepthStencil, NiDepthStencilBuffer* pkDSBuffer, uint32_t a7, uint32_t uiBackgroundColor);

	NiRenderedTexture* GetTexture(const uint32_t uiIndex) const;

	NiRenderTargetGroup* GetRenderTargetGroup() const;

	static void StartOffscreen(uint32_t auiClearMode, NiRenderTargetGroup* apGroup);
	static void StopOffscreen();

	// Static variables used during NiRenderedTexture creation.
	static constexpr AddressPtr<bool, 0x11F4AC0>		bUseCustomFormat;
	static constexpr AddressPtr<bool, 0x11AB3C4>		bIsRenderTarget;

	static constexpr AddressPtr<D3DFORMAT, 0x11AB3C8>	eFormat;
};

ASSERT_SIZE(BSRenderedTexture, 0x40);