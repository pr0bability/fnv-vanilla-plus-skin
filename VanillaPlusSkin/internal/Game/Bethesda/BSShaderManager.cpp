#include "BSShaderManager.hpp"

ShaderBuffer* BSShaderManager::GetShaderBufferLoader() {
	return *reinterpret_cast<ShaderBuffer**>(0x11F9498);
}

NiDX9Renderer* BSShaderManager::GetRenderer() {
	return *reinterpret_cast<NiPointer<NiDX9Renderer>*>(0x11F9508);
}