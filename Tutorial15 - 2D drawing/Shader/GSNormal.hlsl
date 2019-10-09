#include "Header.hlsli"

[maxvertexcount(2)]
void main(point VS_OUTPUT input[1], inout LineStream<VS_OUTPUT> output)
{
	VS_OUTPUT element;

	element = input[0];
	element.Color = float4(1, 1, 0, 1);
	output.Append(element);

	element.Position += input[0].WVPNormal;
	element.Color = float4(1, 0.6f, 0, 1);
	output.Append(element);

	output.RestartStrip();
}