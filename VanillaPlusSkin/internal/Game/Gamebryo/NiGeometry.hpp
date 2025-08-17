#pragma once

#include "NiAVObject.hpp"
#include "NiGeometryData.hpp"
#include "NiSkinInstance.hpp"
#include "NiAlphaProperty.hpp"
#include "NiCullingProperty.hpp"
#include "NiMaterialProperty.hpp"
#include "NiShadeProperty.hpp"
#include "NiStencilProperty.hpp"
#include "NiTexturingProperty.hpp"
#include "NiPropertyState.hpp"

class NiRenderer;
class BSShader;

NiSmartPointer(NiGeometry);

class NiGeometry : public NiAVObject {
public:
	NiGeometry();
	virtual ~NiGeometry();

	virtual void	RenderImmediate(NiRenderer* apRenderer);
	virtual void	OnlyRenderImmediate(NiRenderer* apRenderer);
	virtual void	SetModelData(NiGeometryData* apModelData);
	virtual void	CalculateNormals();
	virtual void	CalculateConsistency(bool bTool);

	struct RendererData {};

	NiPropertyState		m_kProperties;
	NiGeometryDataPtr	m_spModelData;
	NiSkinInstancePtr	m_spSkinInstance;
	BSShader*			m_pShader;

	NIRTTI_ADDRESS(0x11F4ACC);

	__forceinline NiGeometryData* GetModelData() const { return m_spModelData.m_pObject; };

	__forceinline NiSkinInstance* GetSkinInstance() const { return m_spSkinInstance.m_pObject; };
	__forceinline void SetSkinInstance(NiSkinInstance* pkSkinInstance) { m_spSkinInstance = pkSkinInstance; };

	__forceinline BSShader* GetShader() const { return m_pShader; };
	__forceinline void SetShader(BSShader* pkShaderDecl) { m_pShader = pkShaderDecl; };

	__forceinline NiAlphaProperty* GetAlphaProperty() const { return m_kProperties.GetAlphaProperty(); };

	__forceinline NiCullingProperty* GetCullingProperty() const { return m_kProperties.GetCullingProperty(); };

	__forceinline NiMaterialProperty* GetMaterialProperty() const { return m_kProperties.GetMaterialProperty(); };

	template <class T>
	T* GetShadeProperty() const { return static_cast<T*>(m_kProperties.GetShadeProperty<T>()); };
	NiShadeProperty* GetShadeProperty() const { return m_kProperties.GetShadeProperty<NiShadeProperty>();};

	__forceinline NiStencilProperty* GetStencilProperty() const { return m_kProperties.GetStencilProperty(); };

	__forceinline NiTexturingProperty* GetTexturingProperty() const { return m_kProperties.GetTexturingProperty(); };
};

ASSERT_SIZE(NiGeometry, 0xC4);