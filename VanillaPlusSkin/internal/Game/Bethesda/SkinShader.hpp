#pragma once

#include "ShadowLightShader.hpp"

class NiD3DVertexShader;
class NiD3DPixelShader;

class SkinShader : public ShadowLightShader {
public:
	virtual void PresetPasses();

	NiD3DVertexShaderPtr	spVertexShaders[26];
	NiD3DPixelShaderPtr		spPixelShaders[13];

	static NiD3DPassPtr* const spPasses;

	static SkinShader* Create(NiDX9ShaderDeclaration* apDecl0, NiDX9ShaderDeclaration* apDecl1);
};

ASSERT_SIZE(SkinShader, 0x128)