// Shared constant buffers for VS

cbuffer cbSpace : register(b0)
{
	float4x4	World;
	float4x4	View;
	float4x4	Projection;
	float4x4	ViewProjection;
	float4x4	WVP;
	float4		EyePosition;
}