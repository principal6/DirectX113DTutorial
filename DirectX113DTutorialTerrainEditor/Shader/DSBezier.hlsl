#include "Header.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 VP;
}

float4 GetBezier(float4 P1, float4 P2, float4 P3, float4 N1, float4 N2, float4 N3, float3 uvw)
{
	float4 w12 = dot((P2 - P1), N1);
	float4 w21 = dot((P1 - P2), N2);

	float4 w23 = dot((P3 - P2), N2);
	float4 w32 = dot((P2 - P3), N3);

	float4 w31 = dot((P1 - P3), N3);
	float4 w13 = dot((P3 - P1), N1);

	float4 b300 = P1;
	float4 b030 = P2;
	float4 b003 = P3;

	float4 b210 = (2 * P1 + P2 - w12 * N1) / 3;
	float4 b120 = (2 * P2 + P1 - w21 * N2) / 3;

	float4 b021 = (2 * P2 + P3 - w23 * N2) / 3;
	float4 b012 = (2 * P3 + P2 - w32 * N3) / 3;

	float4 b102 = (2 * P3 + P1 - w31 * N3) / 3;
	float4 b201 = (2 * P1 + P3 - w13 * N1) / 3;

	float4 E = (b210 + b120 + b021 + b012 + b102 + b201) / 6;
	float4 V = (b300 + b030 + b003) / 3;

	float4 b111 = E + (E - V) / 2;

	float u = uvw.x;
	float v = uvw.y;
	float w = uvw.z;
	float4 Bezier = pow(u, 3) * b300 +
		3 * pow(u, 2) * v * b210 +
		3 * pow(u, 2) * w * b201 +
		3 * u * pow(v, 2) * b120 +
		3 * u * pow(w, 2) * b102 +
		6 * u * v * w * b111 +
		pow(v, 3) * b030 +
		3 * pow(v, 2) * w * b021 +
		3 * v * pow(w, 2) * b012 +
		pow(w, 3) * b003;

	return Bezier;
}

[domain("tri")]
DS_OUTPUT main(HS_CONSTANT_DATA_OUTPUT TessFactors, float3 Domain : SV_DomainLocation, const OutputPatch<HS_OUTPUT, 3> Patch)
{
	DS_OUTPUT Output;

	float4 P1 = Patch[0].WorldPosition;
	float4 P2 = Patch[1].WorldPosition;
	float4 P3 = Patch[2].WorldPosition;

	float4 N1 = normalize(Patch[0].WorldNormal);
	float4 N2 = normalize(Patch[1].WorldNormal);
	float4 N3 = normalize(Patch[2].WorldNormal);

	Output.Position = Output.WorldPosition = GetBezier(P1, P2, P3, N1, N2, N3, Domain);
	Output.Position = mul(float4(Output.Position.xyz, 1), VP);

	Output.Color = Patch[0].Color * Domain.x + Patch[1].Color * Domain.y + Patch[2].Color * Domain.z;
	Output.UV = Patch[0].UV * Domain.x + Patch[1].UV * Domain.y + Patch[2].UV * Domain.z;
	
	Output.WorldNormal = Patch[0].WorldNormal * Domain.x + Patch[1].WorldNormal * Domain.y + Patch[2].WorldNormal * Domain.z;
	Output.WVPNormal = Patch[0].WVPNormal * Domain.x + Patch[1].WVPNormal * Domain.y + Patch[2].WVPNormal * Domain.z;

	return Output;
}
