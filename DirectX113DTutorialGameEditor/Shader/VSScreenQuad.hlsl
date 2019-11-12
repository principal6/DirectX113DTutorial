#include "Deferred.hlsli"

/*
static const float4 KVertices[6] = 
{
	float4(-1.0f, +1.0f, 0.0f, 1.0f),
	float4(+1.0f, +1.0f, 0.0f, 1.0f),
	float4(-1.0f, -1.0f, 0.0f, 1.0f),

	float4(+1.0f, +1.0f, 0.0f, 1.0f),
	float4(+1.0f, -1.0f, 0.0f, 1.0f),
	float4(-1.0f, -1.0f, 0.0f, 1.0f),
};

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;
	Output.Position = KVertices[Input.VertexID];
	Output.TexCoord = float2((Output.Position.x + 1.0f) / 2.0f, (-Output.Position.y + 1.0f) / 2.0f);
	return Output;
}
*/

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;
	Output.Position = Input.Position;
	Output.TexCoord = Input.TexCoord;
	return Output;
}