#include "SubsurfaceScattering.hpp"
#include "Bethesda/BSShader.hpp"
#include "Bethesda/BSRenderedTexture.hpp"
#include "Gamebryo/NiAdditionalGeometryData.hpp"
#include "Gamebryo/NiDX9Renderer.hpp"
#include "Gamebryo/NiTriBasedGeomData.hpp"

using namespace DirectX;

NiD3DPixelShaderPtr SubsurfaceScattering::spScatterProfileShader;
NiD3DPixelShaderPtr SubsurfaceScattering::spSkinLUTShader;

BSRenderedTexturePtr SubsurfaceScattering::spScatterProfileTexture;
BSRenderedTexturePtr SubsurfaceScattering::spSkinLUTTexture;

static const uint32_t uiSSSTexSize = 1024;

void SubsurfaceScattering::AddCurvatureDataToGeometry(NiTriBasedGeom* apGeometry, float afScale) {
    NiTriBasedGeomData* pGeoData = apGeometry->GetModelData();

    if (!pGeoData) {
        LogGeometry(apGeometry, "no geometry data");
        return;
    }

    if (pGeoData->m_spAdditionalGeomData) {
        return;
    }

    const NiPoint3* kVertices = pGeoData->GetVertices();
    const NiPoint3* kNormals = pGeoData->GetNormals();
    const NiPoint3* kBinormals = pGeoData->GetBinormals();
    const NiPoint3* kTangents = pGeoData->GetTangents();

    const uint16_t usTriangleCount = pGeoData->GetTriangleCount();
    const uint16_t usVertexCount = pGeoData->GetVertexCount();

    if (!kVertices || !kNormals || !kTangents || !kBinormals) {
        LogGeometry(apGeometry, "geometry has no vertices or TBN vectors are missing");
        return;
    }

    // Accumulator for each vertex - collects curvature contributions from adjacent faces
    std::vector<std::vector<VertexCurvatureTensor>> vertexAccumulators(usVertexCount);

    for (uint16_t t = 0; t < usTriangleCount; t++) {
        ProcessTriangleForCurvature(pGeoData, t, vertexAccumulators);
    }

    std::vector<FinalVertexCurvature> finalCurvatures(usVertexCount);

    for (uint16_t v = 0; v < usVertexCount; v++) {
        finalCurvatures[v] = AverageCurvatureAtVertex(vertexAccumulators[v]);
    }

    StoreCurvatureResults(pGeoData, finalCurvatures, afScale);
}

void SubsurfaceScattering::InitializeTextures() {
    spScatterProfileShader = BSShader::pLoadPixelShader("SSS\\LINEAR_PROFILE.pso");
    spSkinLUTShader = BSShader::pLoadPixelShader("SSS\\LUT.pso");

    NiTexture::FormatPrefs kTextureFormat(NiTexture::FormatPrefs::FLOAT_COLOR_64, NiTexture::FormatPrefs::ALPHA_DEFAULT, NiTexture::FormatPrefs::NO);
    BSRenderedTexture::bUseCustomFormat = true;
    BSRenderedTexture::bIsRenderTarget = true;
    BSRenderedTexture::eFormat = D3DFMT_A16B16G16R16F;

    spScatterProfileTexture = BSRenderedTexture::Create("LinearScatterProfile", uiSSSTexSize, uiSSSTexSize, &kTextureFormat, Ni2DBuffer::MULTISAMPLE_NONE, false, nullptr, 0, 0);
    spSkinLUTTexture = BSRenderedTexture::Create("SkinLUT", uiSSSTexSize, uiSSSTexSize, &kTextureFormat, Ni2DBuffer::MULTISAMPLE_NONE, false, nullptr, 0, 0);

    RenderSkinLUT();
}

void SubsurfaceScattering::LogGeometry(NiGeometry* apGeometry, const char* asReason) {
    _MESSAGE("Problematic geometry: %s (%s).", apGeometry->GetName(), asReason);
}

void SubsurfaceScattering::ProcessTriangleForCurvature(
    NiTriBasedGeomData* apGeoData,
    uint16_t usTriangleIndex,
    std::vector<std::vector<VertexCurvatureTensor>>& akVertexAccumulators
) {
    uint16_t v0, v1, v2;
    apGeoData->GetTriangleIndices(usTriangleIndex, v0, v1, v2);

    const NiPoint3* pVertices = apGeoData->GetVertices();
    const NiPoint3* pNormals = apGeoData->GetNormals();
    const NiPoint3* pTangents = apGeoData->GetTangents();
    const NiPoint3* pBinormals = apGeoData->GetBinormals();

    FaceCurvatureTensor faceTensor = ComputeFaceCurvatureTensor(
        pVertices[v0], pVertices[v1], pVertices[v2],
        pNormals[v0], pNormals[v1], pNormals[v2]
    );
    VertexCurvatureTensor vertexTensors[3];

    vertexTensors[0] = TransformCurvatureToVertexSpace(faceTensor, pTangents[v0], pBinormals[v0], pNormals[v0]);
    vertexTensors[1] = TransformCurvatureToVertexSpace(faceTensor, pTangents[v1], pBinormals[v1], pNormals[v1]);
    vertexTensors[2] = TransformCurvatureToVertexSpace(faceTensor, pTangents[v2], pBinormals[v2], pNormals[v2]);

    float voronoiAreas[3];
    CalculateVoronoiAreas(pVertices[v0], pVertices[v1], pVertices[v2], voronoiAreas);

    uint16_t vertexIndices[3] = { v0, v1, v2 };
    for (int i = 0; i < 3; i++) {
        vertexTensors[i].weight = voronoiAreas[i];
        akVertexAccumulators[vertexIndices[i]].push_back(vertexTensors[i]);
    }
}

SubsurfaceScattering::FaceCurvatureTensor SubsurfaceScattering::ComputeFaceCurvatureTensor(
    const NiPoint3& arV1, 
    const NiPoint3& arV2, 
    const NiPoint3& arV3,
    const NiPoint3& arN1, 
    const NiPoint3& arN2, 
    const NiPoint3& arN3
) {
    XMVECTOR vertex1 = XMLoadNiPoint3(&arV1);
    XMVECTOR vertex2 = XMLoadNiPoint3(&arV2);
    XMVECTOR vertex3 = XMLoadNiPoint3(&arV3);

    XMVECTOR normal1 = XMLoadNiPoint3(&arN1);
    XMVECTOR normal2 = XMLoadNiPoint3(&arN2);
    XMVECTOR normal3 = XMLoadNiPoint3(&arN3);

    // Calculate edge vectors
    XMVECTOR e1 = XMVectorSubtract(vertex2, vertex1);  // v1 -> v2
    XMVECTOR e2 = XMVectorSubtract(vertex3, vertex2);  // v2 -> v3  
    XMVECTOR e3 = XMVectorSubtract(vertex1, vertex3);  // v3 -> v1

    // Calculate normal differences
    XMVECTOR dn1 = XMVectorSubtract(normal2, normal1);  // n2 - n1
    XMVECTOR dn2 = XMVectorSubtract(normal3, normal2);  // n3 - n2
    XMVECTOR dn3 = XMVectorSubtract(normal1, normal3);  // n1 - n3

    // Create orthonormal coordinate system for the face
    XMVECTOR u = XMVector3Normalize(e1);  // u direction along first edge
    XMVECTOR face_normal = XMVector3Normalize(XMVector3Cross(e1, XMVectorSubtract(vertex3, vertex1)));
    XMVECTOR v = XMVector3Cross(face_normal, u);  // v perpendicular to u in face plane

    // Project edges onto u,v coordinate system
    float e1_u = XMVectorGetX(XMVector3Dot(e1, u));
    float e1_v = XMVectorGetX(XMVector3Dot(e1, v));
    float e2_u = XMVectorGetX(XMVector3Dot(e2, u));
    float e2_v = XMVectorGetX(XMVector3Dot(e2, v));
    float e3_u = XMVectorGetX(XMVector3Dot(e3, u));
    float e3_v = XMVectorGetX(XMVector3Dot(e3, v));

    // Project normal differences onto u,v coordinate system
    float dn1_u = XMVectorGetX(XMVector3Dot(dn1, u));
    float dn1_v = XMVectorGetX(XMVector3Dot(dn1, v));
    float dn2_u = XMVectorGetX(XMVector3Dot(dn2, u));
    float dn2_v = XMVectorGetX(XMVector3Dot(dn2, v));
    float dn3_u = XMVectorGetX(XMVector3Dot(dn3, u));
    float dn3_v = XMVectorGetX(XMVector3Dot(dn3, v));

    // Set up least squares system: 6 equations for 3 unknowns [kappa_uu, kappa_uv, kappa_vv]
    // Each edge gives 2 equations from the tensor equation:
    // [kappa_uu kappa_uv] [e_u] = [dn_u]
    // [kappa_uv kappa_vv] [e_v]   [dn_v]

    float A[18] = {
        // From edge e1: e1_u * kappa_uu + e1_v * kappa_uv = dn1_u
        e1_u, e1_v, 0,
        // From edge e1: e1_u * kappa_uv + e1_v * kappa_vv = dn1_v  
        0, e1_u, e1_v,

        // From edge e2
        e2_u, e2_v, 0,
        0, e2_u, e2_v,

        // From edge e3
        e3_u, e3_v, 0,
        0, e3_u, e3_v
    };

    float b[6] = {
        dn1_u, dn1_v,
        dn2_u, dn2_v,
        dn3_u, dn3_v
    };

    // Solve least squares system
    float kappa_uu, kappa_uv, kappa_vv;
    if (SolveLeastSquares3x3(A, b, kappa_uu, kappa_uv, kappa_vv)) {
        return { u, v, face_normal, kappa_uu, kappa_uv, kappa_vv };
    }
    else {
        return { u, v, face_normal, 0, 0, 0 };
    }
}

bool SubsurfaceScattering::SolveLeastSquares3x3(float A[18], float b[6], float& x1, float& x2, float& x3) {
    // A is 6x3, we need to solve A^T * A * x = A^T * b

    // Calculate A^T * A (3x3 matrix)
    float ATA[9] = { 0 };
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 6; k++) {
                ATA[i * 3 + j] += A[k * 3 + i] * A[k * 3 + j];
            }
        }
    }

    // Calculate A^T * b (3x1 vector)
    float ATb[3] = { 0 };
    for (int i = 0; i < 3; i++) {
        for (int k = 0; k < 6; k++) {
            ATb[i] += A[k * 3 + i] * b[k];
        }
    }

    // Solve 3x3 system using DirectX Math
    XMMATRIX matrix = XMMatrixSet(
        ATA[0], ATA[1], ATA[2], 0,
        ATA[3], ATA[4], ATA[5], 0,
        ATA[6], ATA[7], ATA[8], 0,
        0, 0, 0, 1
    );

    XMVECTOR det;
    XMMATRIX invMatrix = XMMatrixInverse(&det, matrix);

    if (XMVectorGetX(det) > 1e-10f) {
        XMVECTOR rhs = XMVectorSet(ATb[0], ATb[1], ATb[2], 0);
        XMVECTOR solution = XMVector3Transform(rhs, invMatrix);

        x1 = XMVectorGetX(solution);  // kappa_uu
        x2 = XMVectorGetY(solution);  // kappa_uv  
        x3 = XMVectorGetZ(solution);  // kappa_vv

        return true;
    }

    return false;
}

SubsurfaceScattering::VertexCurvatureTensor SubsurfaceScattering::TransformCurvatureToVertexSpace(
    const FaceCurvatureTensor& arFaceTensor,
    const NiPoint3& arVertexTangent, 
    const NiPoint3& arVertexBinormal,
    const NiPoint3& arVertexNormal
) {
    XMVECTOR up = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&arVertexTangent));
    XMVECTOR vp = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&arVertexBinormal));
    XMVECTOR np = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&arVertexNormal));

    // Normalize vertex coordinate system
    up = XMVector3Normalize(up);
    vp = XMVector3Normalize(vp);
    np = XMVector3Normalize(np);

    // Get face coordinate system
    XMVECTOR uf = arFaceTensor.u;
    XMVECTOR vf = arFaceTensor.v;
    XMVECTOR nf = arFaceTensor.n;

    // Check if coordinate systems are coplanar (normals are similar)
    float normalDot = XMVectorGetX(XMVector3Dot(np, nf));
    const float coplanarThreshold = 0.9999f;

    XMVECTOR uf_rotated = uf;
    XMVECTOR vf_rotated = vf;

    if (abs(normalDot) < coplanarThreshold) {
        // Rotate face coordinate system to be coplanar with vertex system
        XMVECTOR rotationAxis = XMVector3Cross(nf, np);
        float axisLength = XMVectorGetX(XMVector3Length(rotationAxis));

        if (axisLength > 1e-6f) {
            rotationAxis = XMVectorScale(rotationAxis, 1.0f / axisLength);

            float cosAngle = XMVectorGetX(XMVector3Dot(nf, np));
            float angle = acosf(std::max(-1.0f, std::min(1.0f, cosAngle)));

            XMMATRIX rotationMatrix = XMMatrixRotationAxis(rotationAxis, angle);

            uf_rotated = XMVector3Transform(uf, rotationMatrix);
            vf_rotated = XMVector3Transform(vf, rotationMatrix);
        }
    }

    // Project vertex coordinates into (rotated) face coordinate system
    float up_dot_uf = XMVectorGetX(XMVector3Dot(up, uf_rotated));
    float up_dot_vf = XMVectorGetX(XMVector3Dot(up, vf_rotated));
    float vp_dot_uf = XMVectorGetX(XMVector3Dot(vp, uf_rotated));
    float vp_dot_vf = XMVectorGetX(XMVector3Dot(vp, vf_rotated));

    // Transform tensor: e = up^T * II * up, f = up^T * II * vp, g = vp^T * II * vp
    float e = (up_dot_uf * up_dot_uf) * arFaceTensor.kappa_uu +
        2.0f * (up_dot_uf * up_dot_vf) * arFaceTensor.kappa_uv +
        (up_dot_vf * up_dot_vf) * arFaceTensor.kappa_vv;

    float f = (up_dot_uf * vp_dot_uf) * arFaceTensor.kappa_uu +
        (up_dot_uf * vp_dot_vf + up_dot_vf * vp_dot_uf) * arFaceTensor.kappa_uv +
        (up_dot_vf * vp_dot_vf) * arFaceTensor.kappa_vv;

    float g = (vp_dot_uf * vp_dot_uf) * arFaceTensor.kappa_uu +
        2.0f * (vp_dot_uf * vp_dot_vf) * arFaceTensor.kappa_uv +
        (vp_dot_vf * vp_dot_vf) * arFaceTensor.kappa_vv;

    return { e, f, g, 0.0f }; // Weight will be set later
}

void SubsurfaceScattering::CalculateVoronoiAreas(const NiPoint3& arV0, const NiPoint3& arV1, const NiPoint3& arV2, float apVoronoiAreas[3]) {
    XMVECTOR vertex0 = XMLoadNiPoint3(&arV0);
    XMVECTOR vertex1 = XMLoadNiPoint3(&arV1);
    XMVECTOR vertex2 = XMLoadNiPoint3(&arV2);

    // Calculate edge vectors
    XMVECTOR edge01 = XMVectorSubtract(vertex1, vertex0);
    XMVECTOR edge02 = XMVectorSubtract(vertex2, vertex0);
    XMVECTOR edge12 = XMVectorSubtract(vertex2, vertex1);

    // Calculate triangle area
    XMVECTOR cross = XMVector3Cross(edge01, edge02);
    float triangleArea = XMVectorGetX(XMVector3Length(cross)) * 0.5f;

    // Check if triangle is obtuse
    float dot01 = XMVectorGetX(XMVector3Dot(edge01, edge02));
    float dot10 = XMVectorGetX(XMVector3Dot(XMVectorNegate(edge01), edge12));
    float dot21 = XMVectorGetX(XMVector3Dot(XMVectorNegate(edge12), XMVectorNegate(edge02)));

    bool obtuse0 = dot01 < 0;
    bool obtuse1 = dot10 < 0;
    bool obtuse2 = dot21 < 0;

    if (obtuse0 || obtuse1 || obtuse2) {
        // Obtuse triangle - use area division
        if (obtuse0) {
            apVoronoiAreas[0] = triangleArea * 0.5f;
            apVoronoiAreas[1] = triangleArea * 0.25f;
            apVoronoiAreas[2] = triangleArea * 0.25f;
        }
        else if (obtuse1) {
            apVoronoiAreas[0] = triangleArea * 0.25f;
            apVoronoiAreas[1] = triangleArea * 0.5f;
            apVoronoiAreas[2] = triangleArea * 0.25f;
        }
        else {
            apVoronoiAreas[0] = triangleArea * 0.25f;
            apVoronoiAreas[1] = triangleArea * 0.25f;
            apVoronoiAreas[2] = triangleArea * 0.5f;
        }
    }
    else {
        // Acute triangle - use cotangent formula
        float edge01_sq = XMVectorGetX(XMVector3LengthSq(edge01));
        float edge02_sq = XMVectorGetX(XMVector3LengthSq(edge02));
        float edge12_sq = XMVectorGetX(XMVector3LengthSq(edge12));

        float crossLength = XMVectorGetX(XMVector3Length(cross));
        if (crossLength > 1e-6f) {
            float cot0 = dot01 / crossLength;
            float cot1 = dot10 / crossLength;
            float cot2 = dot21 / crossLength;

            apVoronoiAreas[0] = (cot1 * edge02_sq + cot2 * edge01_sq) * 0.125f;
            apVoronoiAreas[1] = (cot2 * edge12_sq + cot0 * edge01_sq) * 0.125f;
            apVoronoiAreas[2] = (cot0 * edge12_sq + cot1 * edge02_sq) * 0.125f;

            // Normalize to triangle area
            float totalVoronoiArea = apVoronoiAreas[0] + apVoronoiAreas[1] + apVoronoiAreas[2];
            if (totalVoronoiArea > 1e-6f) {
                float scale = triangleArea / totalVoronoiArea;
                apVoronoiAreas[0] *= scale;
                apVoronoiAreas[1] *= scale;
                apVoronoiAreas[2] *= scale;
            }
        }
        else {
            // Degenerate triangle
            apVoronoiAreas[0] = apVoronoiAreas[1] = apVoronoiAreas[2] = triangleArea / 3.0f;
        }
    }
}

SubsurfaceScattering::FinalVertexCurvature SubsurfaceScattering::AverageCurvatureAtVertex(const std::vector<VertexCurvatureTensor>& akContributions) {
    if (akContributions.empty()) {
        return { 0.0f, 0.0f, 0.0f, 0.0f };
    }

    // Accumulate weighted curvature tensor components
    float totalWeight = 0.0f;
    float avgE = 0.0f, avgF = 0.0f, avgG = 0.0f;

    for (const auto& contrib : akContributions) {
        float weight = contrib.weight;

        avgE += contrib.e * weight;
        avgF += contrib.f * weight;
        avgG += contrib.g * weight;
        totalWeight += weight;
    }

    // Normalize by total weight
    if (totalWeight > 1e-10f) {
        avgE /= totalWeight;
        avgF /= totalWeight;
        avgG /= totalWeight;
    }

    // Mean curvature
    float meanCurvature = (avgE + avgG) * 0.5f;

    return { avgE, avgF, avgG, meanCurvature };  // ku, kuv, kv, mean
}

void SubsurfaceScattering::StoreCurvatureResults(
    NiTriBasedGeomData* apGeoData,
    std::vector<FinalVertexCurvature>& arCurvatures,
    float afScale
) {
    uint16_t usVertexCount = apGeoData->GetVertexCount();
   
    if (afScale != 1.0f) {
        for (auto& curv : arCurvatures) {
            curv.ku *= afScale;
            curv.kuv *= afScale;
            curv.kv *= afScale;
            curv.meanCurvature *= afScale;
        }
    }

    NiAdditionalGeometryData* pAdditionalData = NiAdditionalGeometryData::Create(usVertexCount, 1);
    pAdditionalData->SetDataBlockCount(1);
    pAdditionalData->SetDataStreamCount(1);
    pAdditionalData->SetDataBlock(0, (uint8_t*)arCurvatures.data(), 16 * usVertexCount, true);
    pAdditionalData->SetDataStream(0, 0, 0, NiAdditionalGeometryData::AGD_NITYPE_FLOAT4, usVertexCount, 16, 16);

    apGeoData->SetAdditionalGeomData(pAdditionalData);
    apGeoData->MarkAsChanged(NiGeometryData::ALL_MASK);
}

void SubsurfaceScattering::RenderSkinLUT() {
    NiDX9Renderer* pRenderer = NiDX9Renderer::GetSingleton();
    IDirect3DDevice9* pDevice = pRenderer->GetD3DDevice();

    // Scatter profile.
    BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_NONE, spScatterProfileTexture->GetRenderTargetGroup());

    pDevice->SetPixelShader(spScatterProfileShader->GetShaderHandle());

    pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

    struct Vertex {
        float x, y, z, rhw;
        float u, v;
    };

    Vertex vertices[] = {
        {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f},
        {uiSSSTexSize - 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f},
        {-0.5f, uiSSSTexSize - 0.5f, 0.0f, 1.0f, 0.0f, 0.0f},
        {uiSSSTexSize - 0.5f, uiSSSTexSize - 0.5f, 0.0f, 1.0f, 1.0f, 0.0f}
    };

    pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);

    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));

    BSRenderedTexture::StopOffscreen();
    
    // Skin LUT.
    BSRenderedTexture::StartOffscreen(NiRenderer::CLEAR_NONE, spSkinLUTTexture->GetRenderTargetGroup());

    pDevice->SetPixelShader(spSkinLUTShader->GetShaderHandle());

    pDevice->SetTexture(0, spScatterProfileTexture->GetTexture(0)->GetDX9RendererData()->GetD3DTexture());
    pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);

    pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(Vertex));

    BSRenderedTexture::StopOffscreen();
}
