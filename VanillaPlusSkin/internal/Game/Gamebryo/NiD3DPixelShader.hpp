#pragma once

#include "NiD3DShaderProgram.hpp"

NiSmartPointer(NiD3DPixelShader);

class NiDX9Renderer;

class NiD3DPixelShader : public NiD3DShaderProgram {
public:
	virtual LPDIRECT3DPIXELSHADER9	GetShaderHandle();
	virtual void					SetShaderHandle(LPDIRECT3DPIXELSHADER9 hShader);
	virtual void					DestroyRendererData(LPDIRECT3DPIXELSHADER9 hShader);
	virtual void					RecreateRendererData();

	LPDIRECT3DPIXELSHADER9 m_hShader;

	NIRTTI_ADDRESS(0x126F9AC);
};

ASSERT_SIZE(NiD3DPixelShader, 0x30);