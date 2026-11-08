/* 
 * Based on work by Sascha Willems (Copyright (c) 2025-2026)
 * Modified by Cudakowski, 2026
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

struct MeshObjectData {
    float4x4 modelMatrix;// macierz modelu ( Model space -> World space)
    float4x4 normalMatrix;
};


//struct ten współdzielony zdefiniowany w kodzie (UBO jako wskaźnik dzięki resizable bar :D)
struct ShaderData {
    float4x4 projection; // macierz projekcji( View space -> Screen space (Canonical view volume) )
    float4x4 view;       // macierz widoku (pozycja kamery)( World space -> View space)
    float4 lightPos;
    float4 cameraPos;
    uint32_t selected;
    uint3 _padding;
    MeshObjectData objectData[128];
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

struct PushConstants {
    ShaderData* shaderDataAddress;
    uint32_t objectId;
};

// vertex shader
[shader("vertex")]
VSOutput main(VSInput input, uniform PushConstants push){//, uint instanceIndex: SV_VulkanInstanceID) { // SV_VulkanInstanceID to index instancji modelu obiektu podany przez vulkana
    VSOutput output;
    uint objIdx = push.objectId;

    float4x4 modelMat = push.shaderDataAddress->objectData[objIdx].modelMatrix;
    float4x4 normMat = push.shaderDataAddress->objectData[objIdx].normalMatrix;

    // World Space point position
    float4 worldPos = mul(modelMat, float4(input.Pos.xyz, 1.0));

    // World Space point normal
    output.Normal = normalize(mul((float3x3)normMat, input.Normal));

    // Screen Space point position
    output.Pos = mul(push.shaderDataAddress->projection, mul(push.shaderDataAddress->view, worldPos));
    
    // przekazywane dalej
    output.UV = input.UV;
    output.Factor = (push.shaderDataAddress->selected == objIdx ? 3.0f : 1.0f); // wyróżniamy zaznaczony model
    output.InstanceIndex = objIdx;

    // Wektor od wierzchołka do światła (oba w World Space)
    output.LightVec = push.shaderDataAddress->lightPos.xyz - worldPos.xyz;
    // Wektor od wierzchołka do oka kamery (oba w World Space)
    output.ViewVec = push.shaderDataAddress->cameraPos.xyz - worldPos.xyz;
    return output;
}

//fragment shader!!
[shader("fragment")]
float4 main(VSOutput input) {
    // Phong lighting
    float3 N = normalize(input.Normal);   // World Space point normal
    float3 L = normalize(input.LightVec); // Wektor od wierzchołka do światła (oba w World Space)
    float3 V = normalize(input.ViewVec);  // Wektor od wierzchołka do oka kamery (oba w World Space)
    float3 R = reflect(-L, N);
    float3 diffuse = max(dot(N, L), 0.0025);
    float3 specular = pow(max(dot(R, V), 0.0), 16.0) * 0.75;
    // Sample from texture
    float3 color = textures[NonUniformResourceIndex(input.InstanceIndex)].Sample(input.UV).rgb * input.Factor;// za pomocą descriptor indexingu mamy taki o array tekstur C:
    return float4(diffuse * color.rgb + specular, 1.0);
}

//TODO: Phong na Blinn-Phong
//TODO: Liczenie wektorów w pixel shaderze
//TODO: Dodanie mapowania normalnych