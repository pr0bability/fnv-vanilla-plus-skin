#pragma once

#include "Bethesda/BSRenderedTexture.hpp"
#include "Gamebryo/NiD3DPixelShader.hpp"
#include "Gamebryo/NiTriBasedGeom.hpp"

#include <vector>

class SubsurfaceScattering {
public:
    static NiD3DPixelShaderPtr  spScatterProfileShader;
    static NiD3DPixelShaderPtr  spSkinLUTShader;

    static BSRenderedTexturePtr spScatterProfileTexture;
    static BSRenderedTexturePtr spSkinLUTTexture;

	static void AddCurvatureDataToGeometry(NiTriBasedGeom* apGeometry, float afScale = 1.0f);
    static void InitializeTextures();

protected:
    struct FaceCurvatureTensor {
        DirectX::XMVECTOR u;
        DirectX::XMVECTOR v;
        DirectX::XMVECTOR n;  // Face coordinate system
        float kappa_uu;
        float kappa_uv;
        float kappa_vv;       // Curvature tensor components in face u,v coordinates
    };

    struct VertexCurvatureTensor {
        float e;
        float f;
        float g;        // Curvature tensor components in vertex coordinate system (ku, kuv, kv)
        float weight;   // Voronoi area weight for this contribution
    };

    struct FinalVertexCurvature {
        float ku;            // Curvature in tangent (u) direction
        float kuv;           // Mixed curvature term
        float kv;            // Curvature in binormal (v) direction  
        float meanCurvature; // Mean curvature
    };

    static void ProcessTriangleForCurvature(
        NiTriBasedGeomData* apGeoData,
        uint16_t usTriangleIndex,
        std::vector<std::vector<VertexCurvatureTensor>>& akVertexAccumulators
    );

    static FaceCurvatureTensor ComputeFaceCurvatureTensor(
        const NiPoint3& arV1, 
        const NiPoint3& arV2, 
        const NiPoint3& arV3, 
        const NiPoint3& arN1, 
        const NiPoint3& arN2, 
        const NiPoint3& arN3
    );
    static bool SolveLeastSquares3x3(float A[18], float b[6], float& x1, float& x2, float& x3);
    static VertexCurvatureTensor TransformCurvatureToVertexSpace(
        const FaceCurvatureTensor& arFaceTensor, 
        const NiPoint3& arVertexTangent, 
        const NiPoint3& arVertexBinormal, 
        const NiPoint3& arVertexNormal
    );
    static void CalculateVoronoiAreas(const NiPoint3& arV0, const NiPoint3& arV1, const NiPoint3& arV2, float apVoronoiAreas[3]);
    static FinalVertexCurvature AverageCurvatureAtVertex(const std::vector<VertexCurvatureTensor>& arContributions);

    static void StoreCurvatureResults(NiTriBasedGeomData* apGeoData, std::vector<FinalVertexCurvature>& arCurvatures, float afScale);

    static void RenderSkinLUT();
};
