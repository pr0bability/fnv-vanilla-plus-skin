#pragma once

#include "NiObject.hpp"
#include "NiBound.hpp"
#include "NiColorA.hpp"
#include "NiShaderDeclaration.hpp"

NiSmartPointer(NiGeometryData);

class NiGeometryBufferData;
class NiPoint2;
class NiTriStripsData;
class NiTriShapeData;
class NiAdditionalGeometryData;
class NiGeometryDataIteratorNiColorA;
class NiGeometryDataIteratorNiPoint2;
class NiGeometryDataIteratorNiPoint3;

class NiGeometryData : public NiObject {
public:
	NiGeometryData();
	virtual ~NiGeometryData();

	virtual void				SetActiveVertexCount(uint16_t usActive);
	virtual uint16_t			GetActiveVertexCount() const;
	virtual NiTriStripsData*	IsStripsData() const;
	virtual NiTriShapeData*		IsShapeData() const;
	virtual bool				ContainsVertexData(NiShaderDeclaration::ShaderParameter eParameter) const;
	virtual void				CalculateNormals();

	enum Consistency {
		MUTABLE				= 0x0000,
		STATIC				= 0x4000,
		CONSISTENCY_MASK	= 0x7000,
	};

	enum KeepFlags {
		KEEP_NONE		= 0,
		KEEP_XYZ		= 1 << 0,
		KEEP_NORM		= 1 << 1,
		KEEP_COLOR		= 1 << 2,
		KEEP_UV			= 1 << 3,
		KEEP_INDICES	= 1 << 4,
		KEEP_BONEDATA	= 1 << 5,
		KEEP_ALL		= 0x3F,
	};

	enum DataFlags {
		NBT_METHOD_NONE			= 0x0000,
		NBT_METHOD_NDL			= 0x1000,
		NBT_METHOD_DEPRECATED	= 0x2000,
		NBT_METHOD_ATI			= 0x3000,
		NBT_METHOD_MASK			= 0xF000,
		TEXTURE_SET_MASK		= 0x3F
	};

	enum Compression {
		COMPRESS_NORM		= 1 << 0,
		COMPRESS_COLOR		= 1 << 1,
		COMPRESS_UV			= 1 << 2,
		COMPRESS_WEIGHT		= 1 << 3,
		COMPRESS_POSITION	= 1 << 4,
		COMPRESS_ALL		= 0x1F,
	};

	enum MarkAsChangedFlags {
		VERTEX_MASK		= 1 << 0,
		NORMAL_MASK		= 1 << 1,
		COLOR_MASK		= 1 << 2,
		TEXTURE_MASK	= 1 << 3,
		ALL_MASK		= VERTEX_MASK | NORMAL_MASK | COLOR_MASK | TEXTURE_MASK,
		DIRTY_MASK		= 0xFFF,
	};

	uint16_t							m_usVertices;
	uint16_t							m_usID;
	uint16_t							m_usDataFlags;
	uint16_t							m_usDirtyFlags;
	NiBound								m_kBound;
	NiPoint3*							m_pkVertex;
	NiPoint3*							m_pkNormal;
	NiColorA*							m_pkColor;
	NiPoint2*							m_pkTexture;
	NiPointer<NiAdditionalGeometryData> m_spAdditionalGeomData;
	NiGeometryBufferData*				m_pkBuffData;
	uint8_t								m_ucKeepFlags;
	uint8_t								m_ucCompressFlags;
	bool								m_bVBLocked;
	bool								m_bVBLockWrite;
	bool								bCanSave;

	NIRTTI_ADDRESS(0x11F4884);

	NiPoint3* GetVertices() const;
	NiPoint3* GetNormals() const;
	NiPoint3* GetBinormals() const;
	NiPoint3* GetTangents() const;
	NiColorA* GetColors() const;
	NiPoint2* GetTextures() const;

	uint16_t GetVertexCount() const;

	void SetKeepFlags(uint32_t aeFlags);

	bool LockPackedVertexData(uint32_t abWriteAccess = 1);
	void UnlockPackedVertexData();

	void SetAdditionalGeomData(NiAdditionalGeometryData* apData);

	void MarkAsChanged(uint32_t aeFlags);
};

ASSERT_SIZE(NiGeometryData, 0x40);