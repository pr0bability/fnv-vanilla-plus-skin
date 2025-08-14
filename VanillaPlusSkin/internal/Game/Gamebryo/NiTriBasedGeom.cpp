#include "NiTriBasedGeom.hpp"

NiTriBasedGeomData* NiTriBasedGeom::GetModelData() const {
	if (!m_spModelData) {
		return nullptr;
	}
	return static_cast<NiTriBasedGeomData*>(m_spModelData.m_pObject);
}
