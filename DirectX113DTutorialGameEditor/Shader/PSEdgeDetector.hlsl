#include "Deferred.hlsli"

SamplerState PointSampler : register(s0);
Texture2D DeferredTexture : register(t0);

cbuffer cbScreen : register(b3)
{
	float2 InverseScreenSize;
	float2 Pads;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float SobelX = 0;
	float SobelY = 0;
	for (int y = 0; y < 3; ++y)
	{
		for (int x = 0; x < 3; ++x)
		{
			float2 OffsetTexCoord = Input.TexCoord.xy + float2(x - 1, y - 1) * InverseScreenSize;
			float4 Sampled = DeferredTexture.Sample(PointSampler, OffsetTexCoord).rgba;
			
			float SobelFactor = Sampled.a;
			SobelX += SobelFactor * KSobelKernelX[y][x];
			SobelY += SobelFactor * KSobelKernelY[y][x];
		}
	}

	float Sobel = sqrt(SobelX * SobelX + SobelY * SobelY);
	//float Angle = atan(SobelY / SobelX);

	float4 Result = float4(1, 0.6, 0, Sobel);
	return Result;
}