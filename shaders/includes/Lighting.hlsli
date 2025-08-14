#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "Helpers.hlsli"

// Subsurface scattering related functions.

// Calculate directional curvature from light direction and curvature tensor.
// https://www.glowybits.com/talks/samurai_shading_in_ghost_of_tsushima/#/105
float CurvatureFromLight(
    float3 tangent,
    float3 bitangent,
    float3 curvTensor,
    float3 lightDir
) {
    float2 lightDirProj = float2(dot(lightDir, tangent), dot(lightDir, bitangent));
	
    float curvature = curvTensor.x * (lightDirProj.x * lightDirProj.x) +
	                    2.0f * curvTensor.y * lightDirProj.x * lightDirProj.y +
	                    curvTensor.z * (lightDirProj.y * lightDirProj.y);
                        
    // Scale closer to the LUT level - proper value would be ~0.07 for millimeters, but that produces
    // barely any results, especially in gamma pipeline.
    return curvature * 0.5f;
}

// Calculate subsurface scattering modified NdotL. Simulates diffusion through tiny bumps
// with a softened normal that is blended, and three separate samples of the skin LUT are
// taken.
float3 CalculateSubsurfaceScattering(sampler2D skinLUT, float3 normal, float3 normalSoft, float3 lightDirection, float curvature) {
    float3 normalRed = lerp(normal, normalSoft, 1.0f);
    float3 normalGreen = lerp(normal, normalSoft, 0.6f);
    float3 normalBlue = lerp(normal, normalSoft, 0.3f);
    
    float3 NdotL = float3(
        dot(normalRed, lightDirection),
        dot(normalGreen, lightDirection),
        dot(normalBlue, lightDirection)
    );
    NdotL = NdotL * 0.5f + 0.5f;
    
    float3 SSS;
    SSS.r = tex2Dlod(skinLUT, float4(NdotL.r, 1.0f - curvature, 0.0f, 0.0f)).r;
    SSS.g = tex2Dlod(skinLUT, float4(NdotL.g, 1.0f - curvature, 0.0f, 0.0f)).g;
    SSS.b = tex2Dlod(skinLUT, float4(NdotL.b, 1.0f - curvature, 0.0f, 0.0f)).b;
    
    return SSS;
}

// Lighting.

// Calculates Blinn-Phong lighting with subsurface scattering. Vectors are assumed to be in the same space.
float3 CalculateLighting(
    sampler2D skinLUT,
    float3 diffuse,
    float3 normal,
    float3 normalSoft,
    float3 viewDir,
    float3 lightDir,
    float3 lightColor,
    float3 curvatureTensor,
    float3 vertexTangent,
    float3 vertexBinormal,
    float gloss,
    float specExponent
) {
    lightDir = normalize(lightDir);
    viewDir = normalize(viewDir);
    
    float curvature = CurvatureFromLight(vertexTangent, vertexBinormal, curvatureTensor.xyz, lightDir);
    float3 SSS = CalculateSubsurfaceScattering(skinLUT, normal, normalSoft, lightDir, curvature);

    // We could theoretically de-linearize the SSS here, however, that very noticeably
    // changes the vanilla shading. Decided to keep linear (same as NdotL in vanilla).

    #ifdef SPECULAR
        float3 halfwayDir = normalize(lightDir + viewDir);
    
        float NdotH = max(dot(normal.xyz, halfwayDir.xyz), 0.0f);
    
        float specStrength = gloss * pow(NdotH, specExponent);
        float3 lighting = (diffuse * SSS + specStrength) * lightColor.rgb;
    #else
        float3 lighting = diffuse * SSS * lightColor.rgb;
    #endif
    
    return lighting;
}

#endif