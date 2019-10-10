#include "GameObject3DLine.h"

void CGameObject3DLine::UpdateWorldMatrix()
{
	XMMATRIX Translation{ XMMatrixTranslationFromVector(ComponentTransform.Translation) };
	XMMATRIX Rotation{ XMMatrixRotationQuaternion(ComponentTransform.RotationQuaternion) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(ComponentTransform.Scaling) };

	ComponentTransform.MatrixWorld = Scaling * Rotation * Translation;
}
