#include "DeferredLight.hlsli"

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	Output.Position = float4(0, 0, 0, 1);
	Output.WorldPosition = Input.WorldPosition;
	Output.Color = Input.Color;
	Output.Range = Input.Range;
	return Output;
}