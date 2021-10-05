//--------------------------------------------------------------------------------------
// File: Tutorial072.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer cbNeverChanges : register(b0)
{
  matrix g_World;
  matrix g_View;
  matrix g_Projection;
};

//--------------------------------------------------------------------------------------
struct VS_INPUT
{
  float4 Pos : POSITION;
};

struct PS_INPUT
{
  float4 Pos : SV_POSITION;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT BASIC_VS(VS_INPUT input)
{
  PS_INPUT output = (PS_INPUT)0;

  output.Pos = mul(input.Pos, g_World);
  output.Pos = mul(output.Pos, g_View);
  output.Pos = mul(output.Pos, g_Projection);

  return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 BASIC_PS(PS_INPUT input) : SV_Target
{
  //result /= lights[0].numberOfLights;
  return float4(1, 0, 0, 1);
}
