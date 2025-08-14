#include "SkinShader.hpp"

NiD3DPassPtr* const SkinShader::spPasses = reinterpret_cast<NiD3DPassPtr*>(0x12022C0);

// GAME - 0xBCF7F0
SkinShader* SkinShader::Create(NiDX9ShaderDeclaration* apDecl0, NiDX9ShaderDeclaration* apDecl1) {
    return NiCreate<SkinShader, 0xBCF7F0>(apDecl0, apDecl1);
}
