#include "iFont.hlsli"

cbuffer cbSpace : register(b1)
{
	float4x4	g_Projection;
	float4		g_Position;
};

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Input.Position.x += g_Position.x;
	Input.Position.y -= g_Position.y;
	Input.Position.w = 1;
	Output.Position = mul(Input.Position, g_Projection);
	Output.TexCoord = Input.TexCoord;

	return Output;
}