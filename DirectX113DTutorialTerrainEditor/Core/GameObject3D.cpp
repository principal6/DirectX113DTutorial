#include "GameObject3D.h"
#include "Game.h"

void CGameObject3D::UpdateWorldMatrix()
{
	if (ComponentTransform.Pitch > CGame::KRotationMaxLimit) ComponentTransform.Pitch = CGame::KRotationMinLimit;
	if (ComponentTransform.Pitch < CGame::KRotationMinLimit) ComponentTransform.Pitch = CGame::KRotationMaxLimit;

	if (ComponentTransform.Yaw > CGame::KRotationMaxLimit) ComponentTransform.Yaw = CGame::KRotationMinLimit;
	if (ComponentTransform.Yaw < CGame::KRotationMinLimit) ComponentTransform.Yaw = CGame::KRotationMaxLimit;

	if (ComponentTransform.Roll > CGame::KRotationMaxLimit) ComponentTransform.Roll = CGame::KRotationMinLimit;
	if (ComponentTransform.Roll < CGame::KRotationMinLimit) ComponentTransform.Roll = CGame::KRotationMaxLimit;

	if (XMVectorGetX(ComponentTransform.Scaling) < CGame::KScalingMinLimit)
		ComponentTransform.Scaling = XMVectorSetX(ComponentTransform.Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetY(ComponentTransform.Scaling) < CGame::KScalingMinLimit)
		ComponentTransform.Scaling = XMVectorSetY(ComponentTransform.Scaling, CGame::KScalingMinLimit);
	if (XMVectorGetZ(ComponentTransform.Scaling) < CGame::KScalingMinLimit)
		ComponentTransform.Scaling = XMVectorSetZ(ComponentTransform.Scaling, CGame::KScalingMinLimit);

	XMMATRIX Translation{ XMMatrixTranslationFromVector(ComponentTransform.Translation) };
	XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(ComponentTransform.Pitch,
		ComponentTransform.Yaw, ComponentTransform.Roll) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(ComponentTransform.Scaling) };

	ComponentTransform.MatrixWorld = Scaling * Rotation * Translation;
}