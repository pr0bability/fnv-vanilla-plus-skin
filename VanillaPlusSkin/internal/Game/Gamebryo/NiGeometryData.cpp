#include "NiGeometryData.hpp"
#include "NiAdditionalGeometryData.hpp"

NiPoint3* NiGeometryData::GetVertices() const {
	return m_pkVertex;
}

NiPoint3* NiGeometryData::GetNormals() const {
	return m_pkNormal;
}

NiPoint3* NiGeometryData::GetBinormals() const {
	return m_pkNormal + m_usVertices;
}

NiPoint3* NiGeometryData::GetTangents() const {
	return m_pkNormal + (2 * m_usVertices);
}

NiColorA* NiGeometryData::GetColors() const {
	return m_pkColor;
}

NiPoint2* NiGeometryData::GetTextures() const {
	return m_pkTexture;
}

uint16_t NiGeometryData::GetVertexCount() const {
	return m_usVertices;
}

void NiGeometryData::SetKeepFlags(uint32_t aeFlags) {
	m_ucKeepFlags = static_cast<KeepFlags>(aeFlags);
}

// GAME - 0xA67360
bool NiGeometryData::LockPackedVertexData(uint32_t abWriteAccess) {
	return ThisCall<bool>(0xA67360, this, abWriteAccess);
}

// GAME - 0xA673D0
void NiGeometryData::UnlockPackedVertexData() {
	ThisCall(0xA673D0, this);
}

void NiGeometryData::SetAdditionalGeomData(NiAdditionalGeometryData* apData) {
	//m_spAdditionalGeomData = apData;
	ThisCall(0xA67260, this, apData);
}

// GAME - 0xA67090
// GECK - 0x80C200
void NiGeometryData::MarkAsChanged(uint32_t aeFlags) {
	NiAdditionalGeometryData* pAdditionalData = m_spAdditionalGeomData;
	if (!pAdditionalData || !IS_PACKED(pAdditionalData))
		m_usDirtyFlags |= aeFlags & DIRTY_MASK;
}
