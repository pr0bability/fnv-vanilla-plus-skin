#include "nvse/PluginAPI.h"
#include "Bethesda/BSMemory.hpp"
#include "Bethesda/BSShader.hpp"
#include "Bethesda/BSShaderPPLightingProperty.hpp"
#include "Bethesda/SkinShader.hpp"
#include "Gamebryo/NiDX9Renderer.hpp"
#include "SubsurfaceScattering.hpp"

BS_ALLOCATORS

IDebugLog gLog("logs\\VanillaPlusSkin.log");

// Hook related globals.
static VirtFuncDetour kPickShaderDetour;
static VirtFuncDetour kSetupGeometryDetour;
static VirtFuncDetour kSkinUpdateConstants;

// Constants.
static NVSEMessagingInterface*	pMsgInterface = nullptr;
static uint32_t					uiPluginHandle = 0;
static constexpr uint32_t		uiShaderLoaderVersion = 131;

class BSShaderPPLightingPropertyEx : public BSShaderPPLightingProperty {
public:
	BSShaderProperty* PickShaderEx(NiGeometry* apGeometry, void* unk0, void* unk1) {
		// Force shader to SLS if shader is skin and doesn't have the facegen flag.
		if (uiShaderIndex == BSShaderManager::BSSM_SHADER_SKIN && !IsFaceGen()) {
			SubsurfaceScattering::LogGeometry(apGeometry, "skin shader on not facegen geometry");
			uiShaderIndex = BSShaderManager::BSSM_SHADER_SHADOWLIGHT;
		}

		return ThisCall<BSShaderProperty*>(kPickShaderDetour.GetOverwrittenAddr(), this, apGeometry, unk0, unk1);
	};

	bool SetupGeometryEx(NiGeometry* apGeometry) {
		// Setup curvature for anything using skin shader (faces, hands, etc.).
		NiTriBasedGeom* pTriBasedGeom = apGeometry->IsTriBasedGeometry();
		if (pTriBasedGeom && apGeometry->GetShader()->IsSkinShader())
			SubsurfaceScattering::AddCurvatureDataToGeometry(pTriBasedGeom);

		return ThisCall<bool>(kSetupGeometryDetour.GetOverwrittenAddr(), this, apGeometry);
	}
};

class SkinShaderEx : public SkinShader {
public:
	// GAME - 0xBCFB00
	static SkinShader* CreateSDInfo() {
		NiDX9Renderer* pRenderer = NiDX9Renderer::GetSingleton();

		NiDX9ShaderDeclaration* pDecl0 = NiDX9ShaderDeclaration::Create(pRenderer, 7, 1);
		NiDX9ShaderDeclaration* pDecl1 = NiDX9ShaderDeclaration::Create(pRenderer, 9, 1);

		pDecl0->SetEntry(0, 0, NiShaderDeclaration::NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::NiShaderDeclaration::SPTYPE_FLOAT3, 0);
		pDecl0->SetEntry(1u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
		pDecl0->SetEntry(2u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
		pDecl0->SetEntry(3u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
		pDecl0->SetEntry(4u, 0xEu, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
		pDecl0->SetEntry(5u, 0xFu, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
		pDecl0->SetEntryAlt(0, 6u, 8u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK, NiShaderDeclaration::SPTYPE_FLOAT4, NiShaderDeclaration::SPUSAGE_TEXCOORD, 1u, NiD3DShaderDeclaration::SPTESS_DEFAULT);
		pDecl0->GetD3DDeclaration();

		pDecl1->SetEntry(0, 0, NiShaderDeclaration::SHADERPARAM_NI_POSITION, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
		pDecl1->SetEntry(1u, 1u, NiShaderDeclaration::SHADERPARAM_NI_BLENDWEIGHT, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
		pDecl1->SetEntry(2u, 2u, NiShaderDeclaration::SHADERPARAM_NI_BLENDINDICES, NiShaderDeclaration::SPTYPE_UBYTECOLOR, 0);
		pDecl1->SetEntry(3u, 3u, NiShaderDeclaration::SHADERPARAM_NI_NORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
		pDecl1->SetEntry(4u, 5u, NiShaderDeclaration::SHADERPARAM_NI_COLOR, NiShaderDeclaration::SPTYPE_FLOAT4, 0);
		pDecl1->SetEntry(5u, 7u, NiShaderDeclaration::SHADERPARAM_NI_TEXCOORD0, NiShaderDeclaration::SPTYPE_FLOAT2, 0);
		pDecl1->SetEntry(6u, 0xEu, NiShaderDeclaration::SHADERPARAM_NI_TANGENT, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
		pDecl1->SetEntry(7u, 0xFu, NiShaderDeclaration::SHADERPARAM_NI_BINORMAL, NiShaderDeclaration::SPTYPE_FLOAT3, 0);
		pDecl1->SetEntryAlt(0, 8u, 8u, NiShaderDeclaration::SHADERPARAM_EXTRA_DATA_MASK, NiShaderDeclaration::SPTYPE_FLOAT4, NiShaderDeclaration::SPUSAGE_TEXCOORD, 1u, NiD3DShaderDeclaration::SPTESS_DEFAULT);
		pDecl1->GetD3DDeclaration();
		
		SkinShader* pShader = SkinShader::Create(pDecl0, pDecl1);
		pShader->Initialize();
		pShader->LoadShaders();
		pShader->LoadStagesAndPasses();
		pShader->CreateConstantMaps();
		pShader->SetShaderDecl(pDecl0);
		return pShader;
	}

	// GAME - 0xB7C1F0
	void SetGlowMap(void* apShaderProp, uint32_t auiStageNumber, uint32_t auiTextureNumber, bool abFallbackTex) {
		NiD3DTextureStage* pTextureStage = GetCurrentPass()->GetStage(auiStageNumber);

		if (pTextureStage->m_pkTexture == SubsurfaceScattering::spSkinLUTTexture)
			return;

		pTextureStage->m_pkTexture = SubsurfaceScattering::spSkinLUTTexture;
		pTextureStage->SetClampMode(NiTexturingProperty::CLAMP_S_CLAMP_T);
	}

	// GAME - 0xB7DAB0
	void UpdateConstants(void* apProperties) {
		ThisCall(kSkinUpdateConstants.GetOverwrittenAddr(), this, apProperties);

		// Pass the vertex shader light data to pixel shaders too.
		m_pkD3DDevice->SetPixelShaderConstantF(50, (float*)0x11FD9A8, 10);
	}
};

void InitHooks() {
	kPickShaderDetour.ReplaceVirtualFuncEx(0x10AE180, &BSShaderPPLightingPropertyEx::PickShaderEx);
	kSetupGeometryDetour.ReplaceVirtualFuncEx(0x10AE16C, &BSShaderPPLightingPropertyEx::SetupGeometryEx);

	WriteRelJump(0xBCFB00, &SkinShaderEx::CreateSDInfo);
	ReplaceCallEx(0xBD036B, &SkinShaderEx::SetGlowMap);
	kSkinUpdateConstants.ReplaceVirtualFuncEx(0x10BB9FC, &SkinShaderEx::UpdateConstants);
}

void MessageHandler(NVSEMessagingInterface::Message* msg) {
	switch (msg->type) {
	case NVSEMessagingInterface::kMessage_DeferredInit:
		// Render skin SSS LUT.
		SubsurfaceScattering::InitializeTextures();

		// Add device reset callback for texture initialization.
		NiDX9Renderer::GetSingleton()->AddResetNotificationFunc(SubsurfaceScattering::ResetCallback, nullptr);
		break;
	}
}

EXTERN_DLL_EXPORT bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "Vanilla Plus Skin";
	info->version = 111;

	return !nvse->isEditor;
}

EXTERN_DLL_EXPORT bool NVSEPlugin_Load(NVSEInterface* nvse) {
	HMODULE hShaderLoader = GetModuleHandle("Fallout Shader Loader.dll");
	HMODULE hLODFlickerFix = GetModuleHandle("LODFlickerFix.dll");

	if (!hShaderLoader) {
		MessageBox(NULL, "Fallout Shader Loader not found.\nVanilla Plus Skin cannot be used without it, please install it.", "Vanilla Plus Skin", MB_OK | MB_ICONERROR);
		ExitProcess(0);
	}

	if (!hLODFlickerFix) {
		MessageBox(NULL, "LOD Flicker Fix not found.\nVanilla Plus Skin cannot be used without it, please install it.", "Vanilla Plus Skin", MB_OK | MB_ICONERROR);
		ExitProcess(0);
	}

	pMsgInterface = (NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging);
	uiPluginHandle = nvse->GetPluginHandle();
	pMsgInterface->RegisterListener(uiPluginHandle, "NVSE", MessageHandler);

	auto pQuery = (_NVSEPlugin_Query)GetProcAddress(hShaderLoader, "NVSEPlugin_Query");
	PluginInfo kInfo = {};
	pQuery(nvse, &kInfo);
	if (kInfo.version < uiShaderLoaderVersion) {
		char cBuffer[192];
		sprintf_s(cBuffer, "Fallout Shader Loader is outdated.\nPlease update it to use Vanilla Plus Skin!\nCurrent version: %i\nMinimum required version: %i", kInfo.version, uiShaderLoaderVersion);
		MessageBox(NULL, cBuffer, "Vanilla Plus Skin", MB_OK | MB_ICONERROR);
		ExitProcess(0);
	}

	BSShader::pLoadPixelShader = (pfn_CreatePixelShader*)GetProcAddress(hShaderLoader, "CreatePixelShader");

	if (!BSShader::pLoadPixelShader) {
		MessageBox(NULL, "Failed to load shader loader functions.", "Vanilla Plus Skin", MB_OK | MB_ICONERROR);
		ExitProcess(0);
	}

	InitHooks();

	return true;
}

BOOL WINAPI DllMain(
	HANDLE  hDllHandle,
	DWORD   dwReason,
	LPVOID  lpreserved
)
{
	return TRUE;
}