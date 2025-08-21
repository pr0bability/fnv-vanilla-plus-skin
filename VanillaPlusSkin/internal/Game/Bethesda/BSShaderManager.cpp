#include "BSShaderManager.hpp"

ShaderBuffer* BSShaderManager::GetShaderBufferLoader() {
	return *reinterpret_cast<ShaderBuffer**>(0x11F9498);
}

NiDX9Renderer* BSShaderManager::GetRenderer() {
	return *reinterpret_cast<NiPointer<NiDX9Renderer>*>(0x11F9508);
}

// GAME - 0xB55840
// GECK - 0x8FE510
void BSShaderManager::GetTexture(const char* apFilename, bool abLoadFile, NiPointer<NiTexture>& aspTexture, bool abCheckMips, bool abCubeMap) {
	CdeclCall(0xB55840, apFilename, abLoadFile, &aspTexture, abCheckMips, abCubeMap);
}
