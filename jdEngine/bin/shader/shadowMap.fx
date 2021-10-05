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
  float4 Depth : TEXCOORD0;
};

float Lamber_Diffuse(in float3 lightDir, in float3 surfNormal)
{
  return max(0.0f, dot(lightDir, surfNormal));
}

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

  input.Pos.w = 1;
  float4 position = mul(input.Pos, boneTrans);
  // Change the position vector to be 4 units for proper matrix calculations.
  //position.w = 1.0f;

  // Calculate the position of the vertex against the world, view, and projection matrices.
  output.Pos = mul(position, World);
  output.Pos = mul(output.Pos, depthMVP);
  //output.Pos = mul(output.Pos, WorldViewInv);
  //output.Pos = mul(output.Pos, WorldViewProjInv);


  output.Depth = output.Pos;
  return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 ShadowMap(PS_INPUT input) : SV_Target
{
  float depthValue;
	float4 color;
  //return float4(1,0,0, 1.0f);
  depthValue = input.Depth.z / input.Depth.w;

	color = float4(depthValue, depthValue, depthValue, 1.0f);

	return color;
}
