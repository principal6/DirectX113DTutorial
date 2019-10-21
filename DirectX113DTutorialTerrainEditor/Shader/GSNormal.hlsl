#include "Header.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 VP;
}

[maxvertexcount(30)]
void main(triangle VS_OUTPUT Input[3], inout TriangleStream<VS_OUTPUT> Output)
{
	Output.Append(Input[0]);
	Output.Append(Input[1]);
	Output.Append(Input[2]);
	Output.RestartStrip();

	VS_OUTPUT Vertex;

	// Normal representation
	for (int i = 0; i < 3; ++i)
	{
		Vertex = Input[i];
		Vertex.Color = float4(1, 1, 0, 1);
		Vertex.bUseVertexColor = 1;

		Output.Append(Vertex);

		Vertex.Position = Vertex.WorldPosition + normalize(Input[i].WorldNormal);
		Vertex.Position = float4(Vertex.Position.xyz, 1);
		Vertex.Position = mul(Vertex.Position, VP);

		Vertex.Color = float4(1, 0.6f, 0, 1);

		Output.Append(Vertex);
		Output.Append(Vertex);

		Output.RestartStrip();
	}

	// Tangent representation
	for (int j = 0; j < 3; ++j)
	{
		Vertex = Input[j];
		Vertex.Color = float4(1, 0, 0, 1);
		Vertex.bUseVertexColor = 1;

		Output.Append(Vertex);

		Vertex.Position = Vertex.WorldPosition + normalize(Input[j].WorldTangent);
		Vertex.Position = float4(Vertex.Position.xyz, 1);
		Vertex.Position = mul(Vertex.Position, VP);

		Output.Append(Vertex);
		Output.Append(Vertex);

		Output.RestartStrip();
	}

	// Bitangent representation
	for (int k = 0; k < 3; ++k)
	{
		Vertex = Input[k];
		Vertex.Color = float4(0, 0, 1, 1);
		Vertex.bUseVertexColor = 1;

		Output.Append(Vertex);

		Vertex.Position = Vertex.WorldPosition + normalize(Input[k].WorldBitangent);
		Vertex.Position = float4(Vertex.Position.xyz, 1);
		Vertex.Position = mul(Vertex.Position, VP);

		Output.Append(Vertex);
		Output.Append(Vertex);

		Output.RestartStrip();
	}
}