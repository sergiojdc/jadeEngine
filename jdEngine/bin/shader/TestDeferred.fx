//--------------------------------------------------------------------------------------
// File: Tutorial072.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
#define M_PI 3.1415926538
#define EPSILON 0.00001


Texture2D txDiffuse : register(t0);
Texture2D txNormal : register(t1);
Texture2D txSpecularMetal : register(t2);
Texture2D txRoughness : register(t3);
//TextureCube cubeMap : register(t4);
//
//
SamplerState samLinear : register(s0);
struct lightStruct
{
  float4 lightDirection;
  float4 lightPosition;
  float4 lightColor;
  uint type;//0= directional 1=point 2= spot
  uint numberOfLights;
};

cbuffer cbNeverChanges : register(b0)
{
  matrix View;
  matrix ViewInv;
};

cbuffer cbChangeOnResize : register(b1)
{
  matrix Projection;
  matrix ProjectionInv;
  matrix ViewProjection;
  matrix ViewProjectionInv;
};

cbuffer cbChangesEveryFrame : register(b2)
{
  matrix World;
  matrix WorldInv;
  matrix WorldView;
  matrix WorldViewInv;
  matrix WorldProj;
  matrix WorldProjInv;
  matrix WorldViewProj;
  matrix WorldViewProjInv;
  matrix depthMVP;
  float4 viewPosition;
  float4 vMeshColor;
};

cbuffer cbBonesTransform : register (b3)
{
  matrix boneTransform[200];
};

cbuffer cbLights : register (b4)
{
  lightStruct lights[20];
};

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
  float4 Pos : POSITION;
  float4 Norm : NORMAL0;
  float4 Tang : TANGENT0;
  float2 Tex : TEXCOORD0;
  float4 Color : COLOR0;
  uint4 BonesIDs : BLENDINDICES0;
  float4 Weights : BLENDWEIGHT0;
};

struct PS_INPUT
{
  float4 Pos : SV_POSITION;
  float3 worldPos : COLOR0;
  float3 viewPos : COLOR1;
  float2 Tex : TEXCOORD0;
  float3x3 TBN : TEXCOORD1;
  float4 depth : COLOR2;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{
  PS_INPUT output = (PS_INPUT)0;
  matrix boneTrans = boneTransform[input.BonesIDs[0]] * input.Weights[0];
  boneTrans += boneTransform[input.BonesIDs[1]] * input.Weights[1];
  boneTrans += boneTransform[input.BonesIDs[2]] * input.Weights[2];
  boneTrans += boneTransform[input.BonesIDs[3]] * input.Weights[3];
  float4 position = mul(input.Pos, boneTrans);

  output.Pos = mul(position, World);
  output.worldPos = output.Pos.xyz;
  output.Pos = mul(output.Pos, View);
  output.viewPos = output.Pos.xyz;
  output.Pos = mul(output.Pos, Projection);

  matrix boneWV = mul(mul(boneTrans, World), View);

  //tangent
  output.TBN[0] = normalize(mul(input.Tang, boneWV)).xyz;
  //normal
  output.TBN[2] = normalize(mul(input.Norm, boneWV)).xyz;
  //binormal
  output.TBN[1] = normalize(cross(output.TBN[2], output.TBN[0]));
  output.Tex = input.Tex;

  output.depth = distance(viewPosition.xyz, output.Pos.xyz);
  return output;
}

struct PS_OUTPUT
{
  float4 Positions : SV_Target0;
  float4 Normal : SV_Target1;
  float4 Albedo : SV_Target2;
  float4 Depth : SV_Target3;
};
//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
PS_OUTPUT PS(PS_INPUT input)
{
  PS_OUTPUT output = (PS_OUTPUT)0;

  float4 diffuseColor = txDiffuse.Sample(samLinear, input.Tex.xy);
  float4 normalColor = txNormal.Sample(samLinear, input.Tex.xy);
  float4 specularColor = txSpecularMetal.Sample(samLinear, input.Tex.xy);
  float4 roughnessColor = txRoughness.Sample(samLinear, input.Tex.xy);

  float metallic = specularColor.r;
  float roughness = roughnessColor.r;

  //finalColor.xyz = pow(abs(diffuseColor.xyz), 2.2f);

  float3 normal = 2.0f * normalColor.xyz - 1.0f;
  normal = normalize(mul(normal, input.TBN));

  //float3 P = mul(input.worldPos, View).xyz;
  output.Positions = float4(input.viewPos.xyz,1);
  //output.Positions = float4(input.worldPos.xyz,1);
  //float3 nomView = normalize((float4(normal.xyz,0), View).xyz);
  //output.Normal = float4(normal, metallic);
  output.Normal = float4(normal, 1);
  output.Albedo = float4(diffuseColor.xyz, roughness);
  output.Depth = float4(input.depth.xyz, 1);
 
  return output;
}
