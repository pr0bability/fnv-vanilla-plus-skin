#include "NiAdditionalGeometryData.hpp"

// GAME - 0xBA63E0
// GECK - 0x67E290
NiAdditionalGeometryData* NiAdditionalGeometryData::Create(uint16_t ausVertCount, uint32_t auiObjectCount) {
    return NiCreate<NiAdditionalGeometryData, 0xBA63E0>(ausVertCount, auiObjectCount);
}

// GAME - 0xA72BB0
// GECK - 0x82B030
bool NiAdditionalGeometryData::SetDataBlock(uint32_t uiIndex, uint8_t* pucData, uint32_t uiTotalSize, bool bCopyData) {
    return ThisCall(0xA72BB0, this, uiIndex, pucData, uiTotalSize, bCopyData);
}

// GAME - 0xA72CA0
// GECK - 0x82B120
void NiAdditionalGeometryData::SetDataBlockCount(uint32_t auiCount) {
    m_aDataBlocks.SetSize(auiCount);
}

// GAME - 0xA730E0
// GECK - ?
void NiAdditionalGeometryData::SetDataStreamCount(uint32_t auiCount) {
    ThisCall(0xA730E0, this, auiCount);
}

// GAME - 0xA726A0
// GECK - 0x82AB20
bool NiAdditionalGeometryData::SetDataStream(uint32_t auiStreamIndex, uint32_t auiBlockIndex, uint32_t auiBlockOffset, Types auiType, uint16_t ausCount, uint32_t auiUnitSize, uint32_t auiStride) {
    if (ausCount != m_usVertexCount || auiStreamIndex >= m_uiDataStreamCount)
        return false;

    auto pEntries = m_pkDataStreamEntries;
    if (!pEntries || auiBlockIndex > m_aDataBlocks.m_usSize)
        return false;

    auto pStream = &pEntries[auiStreamIndex];
    pStream->m_uiType = auiType;
    pStream->m_uiBlockIndex = auiBlockIndex;
    pStream->m_uiBlockOffset = auiBlockOffset;
    pStream->m_uiStride = auiStride;
    pStream->m_uiUnitSize = auiUnitSize;
    pStream->m_uiTotalSize = auiUnitSize * ausCount;
    return true;
}
