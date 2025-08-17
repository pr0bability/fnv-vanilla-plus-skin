#pragma once

#include "NiShader.hpp"

class NiDX9Renderer;
class NiDX9RenderState;
class NiD3DShaderDeclaration;

class NiD3DShaderInterface : public NiShader {
public:
	NiD3DShaderInterface();
	virtual ~NiD3DShaderInterface();

	virtual NiD3DShaderDeclaration*		GetShaderDecl() const;
	virtual void						SetShaderDecl(NiD3DShaderDeclaration* pkShaderDecl);
	virtual void						HandleResetDevice(void* pkShaderDecl);
	virtual void						HandleLostDevice();
	virtual void						DestroyRendererData();
	virtual bool						RecreateRendererData();
	virtual bool						GetVSPresentAllPasses();
	virtual bool						GetVSPresentAnyPass();
	virtual bool						GetPSPresentAllPasses();
	virtual bool						GetPSPresentAnyPass();
	virtual bool						GetIsBestImplementation();
	virtual void						SetIsBestImplementation(bool bIsBest);

	LPDIRECT3DDEVICE9	m_pkD3DDevice;
	NiDX9Renderer*		m_pkD3DRenderer;
	NiDX9RenderState*	m_pkD3DRenderState;
	bool				m_bIsBestImplementation;

	NIRTTI_ADDRESS(0x126F75C);

	NiDX9Renderer* GetRenderer() const;
};

ASSERT_SIZE(NiD3DShaderInterface, 0x24);