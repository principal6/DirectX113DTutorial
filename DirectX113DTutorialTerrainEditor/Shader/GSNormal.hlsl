#include "Header.hlsli"

[maxvertexcount(12)]
void main(triangle VS_OUTPUT Input[3], inout TriangleStream<VS_OUTPUT> Output)
{
	Output.Append(Input[0]);
	Output.Append(Input[1]);
	Output.Append(Input[2]);
	Output.RestartStrip();

	VS_OUTPUT Vertex;
	for (int i = 0; i < 3; ++i)
	{
		Vertex = Input[i];
		Vertex.Color = float4(1, 1, 0, 1);
		Output.Append(Vertex);

		Vertex.Position += Input[i].WVPNormal;
		Vertex.Color = float4(1, 0.6f, 0, 1);
		Output.Append(Vertex);

		Output.Append(Vertex);

		Output.RestartStrip();
	}
}