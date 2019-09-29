#include "Header.hlsli"

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;
	output.Position = input.Position;
	output.Color = input.Color;
	return output;
}