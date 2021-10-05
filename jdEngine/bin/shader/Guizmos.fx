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
TextureCube cubeMap : register(t4);


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
  float3 PosWorld : TEXCOORD0;
  float2 Tex : TEXCOORD1;
  float3x3 TBN : TEXCOORD2;
};

float Lamber_Diffuse(in float3 lightDir, in float3 surfNormal)
{
  return max(0.0f, dot(lightDir, surfNormal));
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS_Guizmo(VS_INPUT input)
{
  PS_INPUT output = (PS_INPUT)0;
  output.PosWorld = mul(input.Pos, World).xyz;
  output.Pos = mul(input.Pos, World);
  output.Pos = mul(output.Pos, View);
  output.Pos = mul(output.Pos, Projection);
  return output;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS_Guizmo(PS_INPUT input) : SV_Target
{
  //result /= lights[0].numberOfLights;
  return vMeshColor;
}
