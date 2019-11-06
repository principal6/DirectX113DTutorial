#include "Particle.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 ViewProjection;
}

[maxvertexcount(6)]
void main(point VS_PARTICLE_OUTPUT Input[1], inout TriangleStream<VS_PARTICLE_OUTPUT> Output)
{
	const float4 KCenterPosition = mul(Input[0].Position, ViewProjection);
	const float KHalfWidth = 0.5f;
	const float KHalfHeight = 0.5f;
	bool bUseTexture = false;
	if (Input[0].TexColor.w < 0) bUseTexture = true;

	VS_PARTICLE_OUTPUT Result = Input[0];
	float HalfWidth = KHalfWidth * Input[0].Scaling.x;
	float HalfHeight = KHalfHeight * Input[0].Scaling.y;


	Result.Position = KCenterPosition + float4(-HalfWidth, +HalfHeight, 0, 0);
	if (bUseTexture) Result.TexColor = float4(0, 0, Input[0].TexColor.z, -1);
	Output.Append(Result);

	Result.Position = KCenterPosition + float4(+HalfWidth, +HalfHeight, 0, 0);
	if (bUseTexture) Result.TexColor = float4(1, 0, Input[0].TexColor.z, -1);
	Output.Append(Result);

	Result.Position = KCenterPosition + float4(-HalfWidth, -HalfHeight, 0, 0);
	if (bUseTexture) Result.TexColor = float4(0, 1, Input[0].TexColor.z, -1);
	Output.Append(Result);

	Output.RestartStrip();


	Result.Position = KCenterPosition + float4(+HalfWidth, +HalfHeight, 0, 0);
	if (bUseTexture) Result.TexColor = float4(1, 0, Input[0].TexColor.z, -1);
	Output.Append(Result);

	Result.Position = KCenterPosition + float4(+HalfWidth, -HalfHeight, 0, 0);
	if (bUseTexture) Result.TexColor = float4(1, 1, Input[0].TexColor.z, -1);
	Output.Append(Result);

	Result.Position = KCenterPosition + float4(-HalfWidth, -HalfHeight, 0, 0);
	if (bUseTexture) Result.TexColor = float4(0, 1, Input[0].TexColor.z, -1);
	Output.Append(Result);
	
	Output.RestartStrip();
}