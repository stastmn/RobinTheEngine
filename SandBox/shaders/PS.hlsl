

cbuffer cByffer : register(b0)
{
    float3 ambientColor;
    float ambientStrength;
    
    float3 diffuseColor;
    float diffuseStrength;
    
    float3 lightPosition;
    
    float3 viewPositon;
    float specularStrength;
}

struct PS_INPUT
{
    float4 inPos : SV_POSITION;
    float2 inTexCoord : TEXCOORD;
    float3 inNormal : NORMAL;
    float3 inWorldPos : WORLD_POS;
};

SamplerState samplerState : SAMPLER : register(s0);
Texture2D text : TEXTURE : register(t0);

float4 main(PS_INPUT input) : SV_Target
{
    float3 textureColor = text.Sample(samplerState, input.inTexCoord);
    float3 ambient = ambientColor * ambientStrength;
    
    float3 normal = normalize(input.inNormal);
    float3 lightDir = normalize(lightPosition - input.inWorldPos);
    float intensity = max(dot(normal, lightDir), 0.0f);
    float3 diffuse = intensity * diffuseStrength * diffuseColor;
    
    float3 viewDir = normalize(viewPositon - input.inWorldPos);
    float3 reflectionDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectionDir), 0.f), 32.f);
    float3 specular = specularStrength * spec * diffuseColor;
    
    float3 color = textureColor * (ambient + diffuse + specular);
    return float4(color, 1);
}