#pragma once

#include "SharedHeader.h"
#include "Object3D.h"

static string SerializeXMMATRIX(const XMMATRIX& Matrix)
{
	string Result{};

	for (int iRow = 0; iRow < 4; ++iRow)
	{
		Result += '\t';
		for (int iCol = 0; iCol < 4; ++iCol)
		{
			Result += "[" + to_string(Matrix.r[iRow].m128_f32[iCol]) + "]";
		}
		Result += '\n';
	}
	Result += '\n';

	return Result;
}

static void SerializeNodes(const vector<SModelNode>& vNodes, uint32_t NodeIndex, uint32_t Depth, string& SerializedString)
{
	for (uint32_t i = 0; i < Depth; ++i)
	{
		SerializedString += "_ ";
	}

	auto& Node{ vNodes[NodeIndex] };
	SerializedString += "[" + to_string(Node.Index) + "]";
	SerializedString += "[" + Node.Name + "]";
	if (Node.ParentNodeIndex != -1) SerializedString += "[Parent: " + to_string(Node.ParentNodeIndex) + "]";
	if (Node.bIsBone) SerializedString += "[Bone: " + to_string(Node.BoneIndex) + "]";
	SerializedString += '\n';

	for (auto& iChild : Node.vChildNodeIndices)
	{
		SerializeNodes(vNodes, iChild, Depth + 1, SerializedString);
	}
}