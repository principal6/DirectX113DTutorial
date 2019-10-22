#include "Header.hlsli"

#define LINE_LENGTH 0.3f

cbuffer cbSpace : register(b0)
{
	float4x4 VP;
}

[maxvertexcount(30)]
void main(triangle VS_OUTPUT Input[3], inout TriangleStream<VS_OUTPUT> Output)
{
	const float4 KTangentColor		= float4(1, 0, 0, 1);
	const float4 KBitangentColor	= float4(0, 1, 0, 1);
	const float4 KNormalColor		= float4(0, 0, 1, 1);

	Output.Append(Input[0]);
	Output.Append(Input[1]);
	Output.Append(Input[2]);
	Output.RestartStrip();

	VS_OUTPUT Vertex;

	// Normal representation
	for (int i = 0; i < 3; ++i)
	{
		Vertex = Input[i];
		Vertex.Color = KNormalColor;
		Vertex.bUseVertexColor = 1;
		Output.Append(Vertex);

		Vertex.Position = mul(Vertex.WorldPosition + normalize(Input[i].WorldNormal) * LINE_LENGTH, VP);
		Output.Append(Vertex);
		Output.Append(Vertex);

		Output.RestartStrip();
	}

	// Tangent representation
	for (int j = 0; j < 3; ++j)
	{
		Vertex = Input[j];
		Vertex.Color = KTangentColor;
		Vertex.bUseVertexColor = 1;
		Output.Append(Vertex);

		Vertex.Position = mul(Vertex.WorldPosition + normalize(Input[j].WorldTangent) * LINE_LENGTH, VP);
		Output.Append(Vertex);
		Output.Append(Vertex);

		Output.RestartStrip();
	}

	// Bitangent representation
	for (int k = 0; k < 3; ++k)
	{
		Vertex = Input[k];
		Vertex.Color = KBitangentColor;
		Vertex.bUseVertexColor = 1;
		Output.Append(Vertex);

		Vertex.Position = mul(Vertex.WorldPosition + normalize(Input[k].WorldBitangent) * LINE_LENGTH, VP);		
		Output.Append(Vertex);
		Output.Append(Vertex);

		Output.RestartStrip();
	}
}