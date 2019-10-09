#include "GameObject2D.h"

void CGameObject2D::UpdateWorldMatrix()
{
	XMMATRIX Translation{ XMMatrixTranslationFromVector(ComponentTransform.Translation) };
	XMMATRIX Rotation{ XMMatrixRotationQuaternion(ComponentTransform.RotationQuaternion) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(ComponentTransform.Scaling) };

	ComponentTransform.MatrixWorld = Scaling * Rotation * Translation;
}