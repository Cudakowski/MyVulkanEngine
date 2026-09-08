/* Copyright (c) 2025-2026, Sascha Willems
 *
 * SPDX-License-Identifier: MIT
 *
 */

/*
Slang lets us put all shader stages into a single file. 
That removes the need to duplicate the shader interface 
or having to put that into shared includes. It also makes 
it easier to read (and edit) the shader.
*/

// vertex shader input - no werteksy no. Takie jak vertex layout w kodzie
struct VSInput {
    float3 Pos;
    float3 Normal;
	float2 UV;
};

// TEKSTURYYYY!!!!!!!!! :OOOOOOOOO
Sampler2D textures[];

//struct ten współdzielony zdefiniowany w kodzie (UBO jako wskaźnik dzięki resizable bar :D)
struct ShaderData {
    float4x4 projection;
    float4x4 view;
    float4x4 model[3];
    float4 lightPos;
    uint32_t selected;
};

// vertex shader output
struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
    float3 Factor;
    float3 LightVec;
    float3 ViewVec;
    uint32_t InstanceIndex;
};

// vertex shader
[shader("vertex")]
VSOutput main(VSInput input, uniform ShaderData *shaderData, uint instanceIndex: SV_VulkanInstanceID) { // SV_VulkanInstanceID to index instancji modelu obiektu podany przez vulkana
    VSOutput output;
    float4x4 modelMat = shaderData->model[instanceIndex];
    output.Normal = mul((float3x3)mul(shaderData->view, modelMat), input.Normal);
    output.UV = input.UV;
    output.Pos = mul(shaderData->projection, mul(shaderData->view, mul(modelMat, float4(input.Pos.xyz, 1.0))));
    output.Factor = (shaderData->selected == instanceIndex ? 3.0f : 1.0f); // wyróżniamy zaznaczony model
    output.InstanceIndex = instanceIndex;
    // Calculate view vectors required for lighting
    float4 fragPos = mul(mul(shaderData->view, modelMat), float4(input.Pos.xyz, 1.0));
    output.LightVec = shaderData->lightPos.xyz - fragPos.xyz;
    output.ViewVec = -fragPos.xyz;
    return output;
}

//fragment shader!!
[shader("fragment")]
float4 main(VSOutput input) {
    // Phong lighting
    float3 N = normalize(input.Normal);
    float3 L = normalize(input.LightVec);
    float3 V = normalize(input.ViewVec);
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.0025);
    float3 specular = pow(max(dot(R, V), 0.0), 16.0) * 0.75;
    // Sample from texture
    float3 color = textures[NonUniformResourceIndex(input.InstanceIndex)].Sample(input.UV).rgb * input.Factor;// za pomocą descriptor indexingu mamy taki o array tekstur C:
    return float4(diffuse * color.rgb + specular, 1.0);
}