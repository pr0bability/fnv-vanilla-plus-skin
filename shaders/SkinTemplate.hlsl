// Template for skin lighting shaders.
//
// VS
//
// AD
// SKIN2008 - LIGHTS = 2 (ONLY_LIGHT)
// SKIN2009 - LIGHTS = 2, SKIN (ONLY_LIGHT)
// SKIN2010 - LIGHTS = 2, PROJ_SHADOW (ONLY_LIGHT)
// SKIN2011 - LIGHTS = 2, PROJ_SHADOW, SKIN (ONLY_LIGHT)
// SKIN2012 - LIGHTS = 3 (ONLY_LIGHT)
// SKIN2013 - LIGHTS = 3, SKIN (ONLY_LIGHT)
// SKIN2014 - LIGHTS = 3, PROJ_SHADOW (ONLY_LIGHT)
// SKIN2015 - LIGHTS = 3, PROJ_SHADOW, SKIN (ONLY_LIGHT)
//
// ADTS
// SKIN2000 - Base
// SKIN2001 - SKIN
// SKIN2002 - PROJ_SHADOW
// SKIN2003 - PROJ_SHADOW, SKIN
// SKIN2004 - LIGHTS = 2
// SKIN2005 - LIGHTS = 2, SKIN
// SKIN2006 - LIGHTS = 2, PROJ_SHADOW
// SKIN2007 - LIGHTS = 2, PROJ_SHADOW, SKIN
//
// ADTS10
// SKIN2020 - LIGHTS = 9
// SKIN2021 - LIGHTS = 9, SKIN
// SKIN2022 - LIGHTS = 4
// SKIN2023 - LIGHTS = 4, SKIN
// SKIN2024 - LIGHTS = 4, SPECULAR
// SKIN2025 - LIGHTS = 4, SPECULAR, SKIN
//
// DiffusePt
// SKIN2016 - LIGHTS = 2
// SKIN2017 - LIGHTS = 2, SKIN
// SKIN2018 - LIGHTS = 3
// SKIN2019 - LIGHTS = 3, SKIN
//
// PS
//
// AD
// SKIN2004 - LIGHTS = 2 (ONLY_LIGHT)
// SKIN2005 - LIGHTS = 2, PROJ_SHADOW (ONLY_LIGHT)
// SKIN2006 - LIGHTS = 3 (ONLY_LIGHT)
// SKIN2007 - LIGHTS = 3, PROJ_SHADOW (ONLY_LIGHT)
//
// ADTS
// SKIN2000 - Base
// SKIN2001 - PROJ_SHADOW
// SKIN2002 - LIGHTS = 2
// SKIN2003 - LIGHTS = 2, PROJ_SHADOW
//
// ADTS10
// SKIN2010 - LIGHTS = 9
// SKIN2011 - LIGHTS = 4
// SKIN2012 - LIGHTS = 4, SPECULAR
//
// DiffusePt
// SKIN2008 - LIGHTS = 2 (DIFFUSE)
// SKIN2009 - LIGHTS = 3 (DIFFUSE)

#if defined(__INTELLISENSE__)
    #define PS
    #define LIGHTS 3
#endif

#if defined(DIFFUSE)
    #define ONLY_LIGHT
    #define OPT
#endif

#ifdef ONLY_LIGHT
    #define NO_FOG
    #define NO_VERTEX_COLOR
#endif

#include "includes/Helpers.hlsli"
#include "includes/Lighting.hlsli"

#ifdef SKIN
    #include "includes/Skin.hlsli"
#endif

// Toggles.
#ifndef OPT
    #define useVertexColor Toggles.x
    #define useFog Toggles.y
    #define glossPower Toggles.z
    #define alphaTestRef Toggles.w
#else
    #define useVertexColor 0
    #define useFog 0
    #define glossPower 0
    #define alphaTestRef 0
#endif

struct VS_INPUT {
    float4 position : POSITION;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
    float3 normal : NORMAL;
    float4 uv : TEXCOORD0;
    float4 curvatureTensor : TEXCOORD1;
#ifndef NO_VERTEX_COLOR
    float4 vertexColor : COLOR0;
#endif
#ifdef SKIN
    float3 blendWeight : BLENDWEIGHT;
    float4 blendIndices : BLENDINDICES;
#endif
};

#if defined(VS)

struct VS_OUTPUT {
#ifndef NO_VERTEX_COLOR
    float4 vertexColor : COLOR0;
#endif
#ifndef NO_FOG
    float4 fogColor : COLOR1;
#endif
    float4 sPosition : POSITION;
    float2 uv : TEXCOORD0;
    float4 lPosition : TEXCOORD1;
    float4 viewPosition : TEXCOORD2;
    float4 curvatureTensor : TEXCOORD3;
    float3 tangent : TEXCOORD4;
    float3 binormal : TEXCOORD5;
    float3 normal : TEXCOORD6;
#ifdef PROJ_SHADOW
    float4 shadowUVs : TEXCOORD7;
#endif
};

#ifndef NO_FOG
    float3 FogColor : register(c15);
    float4 FogParam : register(c14);
#endif

#ifndef SKIN
    row_major float4x4 ModelViewProj : register(c0);
#else
    row_major float4x4 SkinModelViewProj : register(c1);
    float4 Bones[54] : register(c44);
#endif

float4 EyePosition : register(c16);

#ifdef PROJ_SHADOW
    row_major float4x4 ShadowProj : register(c18);
    float4 ShadowProjData : register(c22);
    float4 ShadowProjTransform : register(c23);
#endif

VS_OUTPUT main(VS_INPUT IN) {
    VS_OUTPUT OUT;
    
    OUT.uv = IN.uv.xy;
    
    float4 position = IN.position.xyzw;
    
    #ifndef SKIN
        float3x3 tbn = float3x3(IN.tangent.xyz, IN.binormal.xyz, IN.normal.xyz);
    
        OUT.lPosition.xyzw = position.xyzw;
        OUT.sPosition.xyzw = mul(ModelViewProj, position.xyzw);
        OUT.tangent.xyz = normalize(IN.tangent.xyz);
        OUT.binormal.xyz = normalize(IN.binormal.xyz);
        OUT.normal.xyz = normalize(IN.normal.xyz);
    #else
        float4 offset = IN.blendIndices.zyxw * 765.01001;
        float4 blend = IN.blendWeight.xyzz;
        blend.w = 1 - weight(IN.blendWeight.xyz);
        float3x3 tbn = BonesTransformTBN(Bones, offset, blend, IN.tangent, IN.binormal, IN.normal);
    
        position.w = 1;
        position.xyz = BonesTransformPosition(Bones, offset, blend, position);
    
        OUT.lPosition.xyzw = position.xyzw;
        OUT.sPosition.xyzw = mul(SkinModelViewProj, position.xyzw);
        OUT.tangent.xyz = normalize(tbn[0]);
        OUT.binormal.xyz = normalize(tbn[1]);
        OUT.normal.xyz = normalize(tbn[2]);
    #endif
    
    OUT.viewPosition.xyzw = EyePosition.xyzw;
    
    #ifndef NO_VERTEX_COLOR
        OUT.vertexColor = clamp(IN.vertexColor, 0.0f, 1.0f);
    #endif

    #ifndef NO_FOG
        float3 fogPos = OUT.sPosition.xyz;
        fogPos.z = OUT.sPosition.w - fogPos.z;
    
        float fogStrength = 1 - saturate((FogParam.x - length(fogPos)) / FogParam.y);
        fogStrength = log2(fogStrength);
        OUT.fogColor.a = exp2(fogStrength * FogParam.z);
        OUT.fogColor.rgb = FogColor.rgb;
    #endif
    
    #ifdef PROJ_SHADOW
        float shadowParam = dot(ShadowProj[3].xyzw, position.xyzw);
        float2 shadowUV;
        shadowUV.x = dot(ShadowProj[0].xyzw, position.xyzw);
        shadowUV.y = dot(ShadowProj[1].xyzw, position.xyzw);
        OUT.shadowUVs.xy = ((shadowParam * ShadowProjTransform.xy) + shadowUV) / (shadowParam * ShadowProjTransform.w);
        OUT.shadowUVs.zw = ((shadowUV.xy - ShadowProjData.xy) / ShadowProjData.w) * float2(1, -1) + float2(0, 1);
    #endif
    
    OUT.curvatureTensor.xyzw = IN.curvatureTensor.xyzw;

    return OUT;
};

#endif // Vertex shaders.

struct PS_INPUT {
#ifndef NO_VERTEX_COLOR
    float4 vertexColor : COLOR0;
#endif
#ifndef NO_FOG
    float4 fogColor : COLOR1;
#endif
    float4 sPosition : POSITION;
    float2 uv : TEXCOORD0;
    float4 lPosition : TEXCOORD1;
    float4 viewPosition : TEXCOORD2;
    float4 curvatureTensor : TEXCOORD3;
    float3 tangent : TEXCOORD4;
    float3 binormal : TEXCOORD5;
    float3 normal : TEXCOORD6;
#ifdef PROJ_SHADOW
    float4 shadowUVs : TEXCOORD7;
#endif
};

struct PS_OUTPUT {
    float4 color : COLOR0;
};


#if defined(PS) && (!defined(LIGHTS) || LIGHTS < 4)

#if defined(DIFFUSE)
    sampler2D NormalMap : register(s0);
    sampler2D GlowMap : register(s4);
#elif defined(ONLY_LIGHT)
    sampler2D BaseMap : register(s0);
    sampler2D NormalMap : register(s1);
    sampler2D GlowMap : register(s3);
#else
    sampler2D BaseMap : register(s0);
    sampler2D NormalMap : register(s1);
    sampler2D FaceGenMap0 : register(s2);
    sampler2D FaceGenMap1 : register(s3);
    sampler2D GlowMap : register(s4);
#endif

#if !defined(DIFFUSE)
    float4 AmbientColor : register(c1);
#endif

float4 PSLightColor[10] : register(c3);
float4 SunDir : register(c18);
float4 LightPos[3] : register(c19);
float4 LightData[10] : register(c50);

#ifdef PROJ_SHADOW
    #if defined(ONLY_LIGHT)
        sampler2D ShadowMap : register(s5);
        sampler2D ShadowMaskMap : register(s6);
    #else
        sampler2D ShadowMap : register(s6);
        sampler2D ShadowMaskMap : register(s7);
    #endif
#endif

#ifndef OPT
    float4 Toggles : register(c27);
#endif

PS_OUTPUT main(PS_INPUT IN) {
    PS_OUTPUT OUT;
    
    #if !defined(DIFFUSE)
        float4 baseColor = tex2D(BaseMap, IN.uv.xy);
        
        #if !defined(OPT)
            clip(AmbientColor.a >= 1 ? 0 : (baseColor.a - alphaTestRef));
        #endif
        
        #if !defined(ONLY_LIGHT)
            float4 faceGenVar = tex2D(FaceGenMap0, IN.uv.xy);
            float4 faceGenBlend = tex2D(FaceGenMap1, IN.uv.xy);
        
            float3 skinBase = 2.0f * ((expand(faceGenVar.rgb) + baseColor.rgb) * (2.0f * faceGenBlend.rgb));
        #else
            float3 skinBase = 1.0f;
        #endif
    
        #ifndef NO_VERTEX_COLOR
            baseColor.rgb = useVertexColor <= 0 ? skinBase : (skinBase * IN.vertexColor.rgb);
        #else
            baseColor.rgb = skinBase;
        #endif
    #else
        float4 baseColor = 1.0f;
    #endif
    
    float3 viewDir = IN.viewPosition.xyz - IN.lPosition.xyz;
    
    float4 normal = tex2D(NormalMap, IN.uv.xy);
    normal.xyz = normalize(expand(normal.xyz));
    float4 normalSoft = tex2Dbias(NormalMap, float4(IN.uv.xy, 0.0f, 2.0f));
    normalSoft.xyz = normalize(expand(normalSoft.xyz));
    
    float3 vertexTangent = normalize(IN.tangent.xyz);
    float3 vertexBinormal = normalize(IN.binormal.xyz);
    float3 vertexNormal = normalize(IN.normal.xyz);
    
    // Convert to local space.
    float3x3 TBN = float3x3(
        vertexTangent,
        vertexBinormal,
        vertexNormal
    );
    
    normal.xyz = normalize(mul(normal.xyz, TBN));
    normalSoft.xyz = normalize(mul(normalSoft.xyz, TBN));
    
    // Shadows.
    float3 shadowMultiplier = 1;
    #ifdef PROJ_SHADOW
        float3 shadow = tex2D(ShadowMap, IN.shadowUVs.xy).xyz;
        float shadowMask = tex2D(ShadowMaskMap, IN.shadowUVs.zw).x;
        shadowMultiplier = lerp(1, shadow, shadowMask);
    #endif
    
    #if !defined(DIFFUSE)
        float3 lightDir = LightData[0].xyz;
        float3 lighting = CalculateLighting(GlowMap, baseColor.rgb, normal.xyz, normalSoft.xyz, viewDir.xyz, lightDir.xyz, PSLightColor[0].rgb * shadowMultiplier, IN.curvatureTensor.xyz, vertexTangent, vertexBinormal, normal.a, glossPower);
    #else
        // Pointlights only.
        float3 lightDir = LightData[0].xyz - IN.lPosition.xyz;
        float3 lighting = CalculateLighting(GlowMap, baseColor.rgb, normal.xyz, normalSoft.xyz, viewDir.xyz, lightDir.xyz, PSLightColor[0].rgb, IN.curvatureTensor.xyz, vertexTangent, vertexBinormal, normal.a, glossPower);
        lighting *= VanillaAttenuation(lightDir, LightData[0].w);
    #endif
    
    #if !defined(DIFFUSE)
        lighting += AmbientColor.rgb * baseColor.rgb;
    #endif
    
    // Other light sources.
    #if LIGHTS > 1
        lightDir = LightData[1].xyz - IN.lPosition.xyz;
        float att = VanillaAttenuation(lightDir, LightData[1].w);
        lighting += att * CalculateLighting(GlowMap, baseColor.rgb, normal.xyz, normalSoft.xyz, viewDir.xyz, lightDir.xyz, PSLightColor[1].rgb * shadowMultiplier, IN.curvatureTensor.xyz, vertexTangent, vertexBinormal, normal.a, glossPower);
    #endif
    
    #if LIGHTS > 2
        lightDir = LightData[2].xyz - IN.lPosition.xyz;
        att = VanillaAttenuation(lightDir, LightData[2].w);
        lighting += att * CalculateLighting(GlowMap, baseColor.rgb, normal.xyz, normalSoft.xyz, viewDir.xyz, lightDir.xyz, PSLightColor[2].rgb * shadowMultiplier, IN.curvatureTensor.xyz, vertexTangent, vertexBinormal, normal.a, glossPower);
    #endif
    
    float3 finalColor = lighting.rgb;
    
    // Fog.
    #ifndef NO_FOG
        #ifndef OPT
            finalColor.rgb = (useFog <= 0.0 ? finalColor.rgb : lerp(finalColor.rgb, IN.fogColor.rgb, IN.fogColor.a));
        #else
            finalColor.rgb = lerp(finalColor.rgb, IN.fogColor.rgb, IN.fogColor.a);
        #endif
    #endif
    
    OUT.color.rgb = finalColor.rgb;
    
    #if defined(DIFFUSE)
        OUT.color.a = 1;
    #elif defined(ONLY_LIGHT)
        OUT.color.a = baseColor.a;
    #else
        OUT.color.a = baseColor.a * AmbientColor.a;
    #endif

    return OUT;
}

#elif defined(PS) && LIGHTS >= 4

#if LIGHTS > 4
    #define MAX_LIGHTS 5
#elif LIGHTS > 3 && !defined(SPECULAR)
    #define MAX_LIGHTS 3
#else
    #define MAX_LIGHTS 2
#endif

sampler2D BaseMap : register(s0);
sampler2D NormalMap : register(s1);
sampler2D FaceGenMap0 : register(s2);
sampler2D FaceGenMap1 : register(s3);
sampler2D GlowMap : register(s4);

float4 AmbientColor : register(c1);
float4 PSLightColor[10] : register(c3);

float4 EmittanceColor : register(c2);

float4 Toggles : register(c27);

float4 LightData[10] : register(c50);

#define lightsUsed EmittanceColor.a
#define lightOffset 0
#define glossPow Toggles.z

PS_OUTPUT main(PS_INPUT IN) {
    PS_OUTPUT OUT;

    float4 baseColor = tex2D(BaseMap, IN.uv.xy);
        
    clip(AmbientColor.a >= 1 ? 0 : (baseColor.a - alphaTestRef));

    float4 faceGenVar = tex2D(FaceGenMap0, IN.uv.xy);
    float4 faceGenBlend = tex2D(FaceGenMap1, IN.uv.xy);
        
    float3 skinBase = 2.0f * ((expand(faceGenVar.rgb) + baseColor.rgb) * (2.0f * faceGenBlend.rgb));
    baseColor.rgb = useVertexColor <= 0 ? skinBase : (skinBase * IN.vertexColor.rgb);
    
    float3 viewDir = IN.viewPosition.xyz - IN.lPosition.xyz;
    
    float4 normal = tex2D(NormalMap, IN.uv.xy);
    normal.xyz = normalize(expand(normal.xyz));
    float4 normalSoft = tex2Dbias(NormalMap, float4(IN.uv.xy, 0.0f, 2.0f));
    normalSoft.xyz = normalize(expand(normalSoft.xyz));
    
    // Convert to local space.
    float3x3 TBN = float3x3(
        normalize(IN.tangent.xyz),
        normalize(IN.binormal.xyz),
        normalize(IN.normal.xyz)
    );
    
    normal.xyz = normalize(mul(normal.xyz, TBN));
    normalSoft.xyz = normalize(mul(normalSoft.xyz, TBN));
    
    float3 lighting = CalculateLighting(GlowMap, baseColor.rgb, normal.xyz, normalSoft.xyz, viewDir.xyz, LightData[0].xyz, PSLightColor[0].rgb, IN.curvatureTensor.xyz, IN.tangent, IN.binormal, normal.a * LightData[0].w, glossPower);
    
    lighting += AmbientColor.rgb * baseColor.rgb;
    
    [unroll]
    for (int i = 0; i < MAX_LIGHTS - 1; i++) {
        float3 lightDir = LightData[i + 1].xyz - IN.lPosition.xyz;
        float3 contribution = CalculateLighting(GlowMap, baseColor.rgb, normal.xyz, normalSoft.xyz, viewDir.xyz, lightDir.xyz, PSLightColor[i + 1].rgb, IN.curvatureTensor.xyz, IN.tangent, IN.binormal, normal.a * LightData[0].w, glossPower);
        contribution *= VanillaAttenuation(lightDir, LightData[i + 1].w);
        contribution *= EmittanceColor.w < i;
        lighting += contribution;
    }
    
    float3 finalColor = (useFog <= 0.0 ? lighting.rgb : lerp(lighting.rgb, IN.fogColor.rgb, IN.fogColor.a));
    
    OUT.color.rgb = finalColor.rgb;
    OUT.color.a = baseColor.a * AmbientColor.a;

    return OUT;
}

#endif // Pixel shaders.