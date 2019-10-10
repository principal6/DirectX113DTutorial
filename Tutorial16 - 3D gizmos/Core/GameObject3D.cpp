#include "GameObject3D.h"

void CGameObject3D::UpdateWorldMatrix()
{
	if (ComponentTransform.Pitch > XM_2PI) ComponentTransform.Pitch = -XM_2PI;
	if (ComponentTransform.Pitch < -XM_2PI) ComponentTransform.Pitch = XM_2PI;

	if (ComponentTransform.Yaw > XM_2PI) ComponentTransform.Yaw = -XM_2PI;
	if (ComponentTransform.Yaw < -XM_2PI) ComponentTransform.Yaw = XM_2PI;

	if (ComponentTransform.Roll > XM_2PI) ComponentTransform.Roll = -XM_2PI;
	if (ComponentTransform.Roll < -XM_2PI) ComponentTransform.Roll = XM_2PI;

	if (XMVectorGetX(ComponentTransform.Scaling) < KScalingMin) ComponentTransform.Scaling = XMVectorSetX(ComponentTransform.Scaling, KScalingMin);
	if (XMVectorGetY(ComponentTransform.Scaling) < KScalingMin) ComponentTransform.Scaling = XMVectorSetY(ComponentTransform.Scaling, KScalingMin);
	if (XMVectorGetZ(ComponentTransform.Scaling) < KScalingMin) ComponentTransform.Scaling = XMVectorSetZ(ComponentTransform.Scaling, KScalingMin);

	XMMATRIX Translation{ XMMatrixTranslationFromVector(ComponentTransform.Translation) };
	XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(ComponentTransform.Pitch,
		ComponentTransform.Yaw, ComponentTransform.Roll) };
	XMMATRIX Scaling{ XMMatrixScalingFromVector(ComponentTransform.Scaling) };

	ComponentTransform.MatrixWorld = Scaling * Rotation * Translation;
}