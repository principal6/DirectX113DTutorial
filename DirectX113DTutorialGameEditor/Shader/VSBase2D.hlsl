#include "Base2D.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 Projection;
	float4x4 WVP;
}

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.Position = mul(Input.Position, World);
	Output.Position = mul(Output.Position, Projection);
	Output.Position.w = 1.0;
	
	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord;

	return Output;
}