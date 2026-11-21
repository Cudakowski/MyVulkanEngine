/* 
 * Based on work by Sascha Willems (Copyright (c) 2025-2026)
 * Modified by Cudakowski, 2026
 *
 * SPDX-License-Identifier: MIT
 *
 */

struct VSInput {
    float3 Pos;
    float3 Normal;
    float2 UV;
    uint32_t TexID; 
};

Sampler2D textures[];

struct MeshObjectData {
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

struct ShaderData {
    float4x4 projection;
    float4x4 view;
    float4 lightPos;
    float4 cameraPos;
    uint32_t selected;
    uint3 _padding;
    MeshObjectData objectData[128];
};

struct VSOutput {
    float4 Pos : SV_POSITION;
    float3 Normal;
    float2 UV;
    nointerpolation uint32_t TexID; // brak interpolacji dla uint
};

struct BlockPushConstants {
    float4x4 modelMatrix;
    ShaderData* shaderDataAddress;
};

[shader("vertex")]
VSOutput main(VSInput input, uniform BlockPushConstants push) {
    VSOutput output;

    float4 worldPos = mul(push.modelMatrix, float4(input.Pos.xyz, 1.0));

    // Normalne bloków są wyrównane do osi (1,0,0 itd.), a chunk nie jest obracany/skalowany
    output.Normal = normalize(mul((float3x3)push.modelMatrix, input.Normal));

    output.Pos = mul(push.shaderDataAddress->projection, mul(push.shaderDataAddress->view, worldPos));
    
    output.UV = input.UV;
    output.TexID = input.TexID;


    return output;
}

[shader("fragment")]
float4 main(VSOutput input) {
    // Phong lighting
    float3 N = normalize(input.Normal);
    // float3 L = normalize(input.LightVec);
    // float3 V = normalize(input.ViewVec);
    // float3 R = reflect(-L, N);

    float3 sunDir = normalize(float3(0.45, -0.85, 0.30)); // do słońca
    // Oświetlenie kierunkowe (N dot L)
    float ndotl = max(dot(N, sunDir), 0.0);

    float skyFactor = saturate(-N.y * 0.5 + 0.5); // 1.0 dla góry, 0.5 dla boków, 0.0 dla spodu
    float ambient = lerp(0.45, 0.65, skyFactor);

    float3 lightColor = float3(1.0, 0.98, 0.92); // lekko ciepłe światło słoneczne
    float3 lighting = ambient + ndotl * 0.65 * lightColor;

    // float3 diffuse = max(dot(N, L), 0.15);
    // float3 specular = pow(max(dot(R, V), 0.0), 16.0) * 0.25; // mniejszy połysk

    float3 color = textures[NonUniformResourceIndex(input.TexID)].Sample(input.UV).rgb;
    return float4(color * lighting, 1.0);
    //return float4(diffuse * color + specular, 1.0);
}