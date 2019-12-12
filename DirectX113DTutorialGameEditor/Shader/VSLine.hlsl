#include "Line.hlsli"
#include "iVSCBs.hlsli"

VS_LINE_OUTPUT main(VS_LINE_INPUT input)
{
	VS_LINE_OUTPUT output;

	output.Position = mul(input.Position, World);
	output.Position = mul(output.Position, ViewProjection);
	output.Color = input.Color;

	return output;
}