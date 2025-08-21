#include "NiDX9Renderer.hpp"

NiDX9Renderer* NiDX9Renderer::GetSingleton() {
	return *reinterpret_cast<NiDX9Renderer**>(0x11C73B4);
}

LPDIRECT3D9 NiDX9Renderer::GetD3D9() {
	return *reinterpret_cast<LPDIRECT3D9*>(0x126F0D8);
}

LPDIRECT3DDEVICE9 NiDX9Renderer::GetD3DDevice() const {
	return m_pkD3DDevice9;
}

uint32_t NiDX9Renderer::AddResetNotificationFunc(pfn_ResetNotifyFunction apfnNotify, void* pvData) {
	return ThisCall<uint32_t>(0x86BAE0, this, apfnNotify, pvData);
}
