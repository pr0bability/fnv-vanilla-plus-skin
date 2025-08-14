// Shaders for generating SSS LUT.
// SSS_LINEAR_PROFILE.pso - SCATTER_PROFILE
// SSS_LUT.pso - Base

#include "includes/Helpers.hlsli"

static const float MAX_DISTANCE = 15.0f;    // Maximum distance the linear scatter profile covers.
static const float INCREMENT = 0.01;        // Integration angle increment.

// Gaussian for radial scatter profiles.
float Gaussian(float v, float r) {
    return 1.0 / (2.0 * PI * v) * exp(-(r * r) / (2.0f * v));
}

// Radial scatter profile from GPU Gems 3.
float3 ScatterRadial(float r) {
    return Gaussian(0.0064, r) * float3(0.233, 0.455, 0.649) +
           Gaussian(0.0484, r) * float3(0.100, 0.336, 0.344) +
           Gaussian(0.1870, r) * float3(0.118, 0.198, 0.000) +
           Gaussian(0.5670, r) * float3(0.113, 0.007, 0.007) +
           Gaussian(1.9900, r) * float3(0.358, 0.004, 0.000) +
           Gaussian(7.4100, r) * float3(0.078, 0.000, 0.000);
}

#ifdef SCATTER_PROFILE

// Preintegrate the radial scatter profile into a linear profile.
float3 PreintegrateLinearProfile(float r) {
    float3 result = 0;
    float maxT = 15.0;
    int numSamples = 128;
    
    for (int i = 0; i < numSamples; i++) {
        float t = lerp(-maxT, maxT, i / (float) (numSamples - 1));
        float distance = sqrt(r * r + t * t);
        result += ScatterRadial(distance);
    }
    
    return result * (2.0 * maxT) / numSamples;
}

float4 main(float2 uv : TEXCOORD0) : COLOR0 {
    // UV.x maps to distance from 0 to MAX_DISTANCE
    float distance = uv.x * MAX_DISTANCE;
    
    // Generate linear profile
    float3 linearProfile = PreintegrateLinearProfile(distance);
    
    return float4(linearProfile, 1.0);
}

#else

sampler2D LinearScatterProfile : register(s0);  // Assume clamping for U coordinate.

float3 SampleLinearProfile(float distance) {
    // Normalize distance to [0,1] for texture sampling
    float normalizedDist = distance / MAX_DISTANCE;
    
    // Sample the precomputed linear profile
    return tex2Dlod(LinearScatterProfile, float4(normalizedDist, 0.5f, 0.0f, 0.0f)).rgb;
}

float3 PreintegrateSSS(float cosTheta, float r) {
    float theta = acos(cosTheta);
    float3 totalWeights = 0;
    float3 totalLight = 0;

    // Integrate over hemisphere around the curved surface. Split to two loops due to iteration limit.
    float angle = -PI / 2.0f;
    while (angle <= 0) {
        float sampleAngle = theta + angle;
        float diffuse = saturate(cos(sampleAngle));
        
        // Distance light travels through the surface
        float sampleDist = abs(2.0 * r * sin(angle * 0.5));
        
        // Sample the precomputed linear profile
        float3 weights = SampleLinearProfile(sampleDist);
        
        totalWeights += weights;
        totalLight += diffuse * weights;
        angle += INCREMENT;
    }
    
    while (angle <= PI / 2.0) {
        float sampleAngle = theta + angle;
        float diffuse = saturate(cos(sampleAngle));
        
        // Distance light travels through the surface
        float sampleDist = abs(2.0 * r * sin(angle * 0.5));
        
        // Sample the precomputed linear profile
        float3 weights = SampleLinearProfile(sampleDist);
        
        totalWeights += weights;
        totalLight += diffuse * weights;
        angle += INCREMENT;
    }
    
    return totalWeights.x > 0.001 ? totalLight / totalWeights : saturate(cosTheta);
}

float4 main(float2 uv : TEXCOORD0) : COLOR0 {
    float cosTheta = uv.x * 2.0f - 1.0f;
    float curvature = 1 / uv.y;

    float3 sssValue = PreintegrateSSS(cosTheta, curvature);
    
    return float4(sssValue, 1.0f);
}

#endif