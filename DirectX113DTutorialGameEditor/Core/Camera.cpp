#include "Camera.h"

using std::max;
using std::min;

void CCamera::Move(EMovementDirection Direction, float StrideFactor)
{
	XMVECTOR dPosition{};

	if (m_CameraData.eType == EType::FreeLook)
	{
		XMVECTOR Rightward{ XMVector3Normalize(XMVector3Cross(m_CameraData.UpDirection, m_CameraData.Forward)) };

		switch (Direction)
		{
		case EMovementDirection::Forward:
			dPosition = +m_CameraData.Forward * StrideFactor;
			break;
		case EMovementDirection::Backward:
			dPosition = -m_CameraData.Forward * StrideFactor;
			break;
		case EMovementDirection::Rightward:
			dPosition = +Rightward * StrideFactor;
			break;
		case EMovementDirection::Leftward:
			dPosition = -Rightward * StrideFactor;
			break;
		default:
			break;
		}
	}
	else if (m_CameraData.eType == EType::FirstPerson || m_CameraData.eType == EType::ThirdPerson)
	{
		XMVECTOR GroundRightward{ XMVector3Normalize(XMVector3Cross(m_CameraData.BaseUpDirection, m_CameraData.Forward)) };
		XMVECTOR GroundForward{ XMVector3Normalize(XMVector3Cross(GroundRightward, m_CameraData.BaseUpDirection)) };

		switch (Direction)
		{
		case EMovementDirection::Forward:
			dPosition = +GroundForward * StrideFactor;
			break;
		case EMovementDirection::Backward:
			dPosition = -GroundForward * StrideFactor;
			break;
		case EMovementDirection::Rightward:
			dPosition = +GroundRightward * StrideFactor;
			break;
		case EMovementDirection::Leftward:
			dPosition = -GroundRightward * StrideFactor;
			break;
		default:
			break;
		}
	}

	m_CameraData.EyePosition += dPosition;
	m_CameraData.FocusPosition += dPosition;
}

void CCamera::Rotate(int DeltaX, int DeltaY, float RotationFactor)
{
	m_CameraData.Pitch += RotationFactor * DeltaY;
	m_CameraData.Yaw += RotationFactor * DeltaX;

	m_CameraData.Pitch = max(-KPitchLimit, m_CameraData.Pitch);
	m_CameraData.Pitch = min(+KPitchLimit, m_CameraData.Pitch);

	if (m_CameraData.Yaw > XM_PI)
	{
		m_CameraData.Yaw -= XM_2PI;
	}
	else if (m_CameraData.Yaw < -XM_PI)
	{
		m_CameraData.Yaw += XM_2PI;
	}

	XMMATRIX MatrixRotation{ XMMatrixRotationRollPitchYaw(m_CameraData.Pitch, m_CameraData.Yaw, 0) };
	m_CameraData.Forward = XMVector3TransformNormal(m_CameraData.BaseForwardDirection, MatrixRotation);
	XMVECTOR Rightward{ XMVector3Normalize(XMVector3Cross(m_CameraData.BaseUpDirection, m_CameraData.Forward)) };
	XMVECTOR Upward{ XMVector3Normalize(XMVector3Cross(m_CameraData.Forward, Rightward)) };

	m_CameraData.UpDirection = Upward;

	if (m_CameraData.eType == EType::FirstPerson || m_CameraData.eType == EType::FreeLook)
	{
		m_CameraData.FocusPosition = m_CameraData.EyePosition + m_CameraData.Forward;
	}
	else if (m_CameraData.eType == EType::ThirdPerson)
	{
		m_CameraData.EyePosition = m_CameraData.FocusPosition - m_CameraData.Forward * m_CameraData.ZoomDistance;
	}
}

void CCamera::Zoom(int DeltaWheel, float ZoomFactor)
{
	if (m_CameraData.eType == EType::ThirdPerson)
	{
		m_CameraData.ZoomDistance -= DeltaWheel * ZoomFactor;
		m_CameraData.ZoomDistance = max(m_CameraData.ZoomDistance, m_CameraData.MinZoomDistance);
		m_CameraData.ZoomDistance = min(m_CameraData.ZoomDistance, m_CameraData.MaxZoomDistance);

		m_CameraData.EyePosition = m_CameraData.FocusPosition - m_CameraData.Forward * m_CameraData.ZoomDistance;
	}
}

void CCamera::SetEyePosition(const XMVECTOR& Position)
{
	m_CameraData.EyePosition = Position;
	
	Update();
}

void CCamera::SetPitch(float Value)
{
	m_CameraData.Pitch = Value;

	Update();
}

void CCamera::SetYaw(float Value)
{
	m_CameraData.Yaw = Value;

	Update();
}

void CCamera::SetType(EType eType)
{
	m_CameraData.eType = eType;

	Update();
}

void CCamera::SetData(const SCameraData& Data)
{
	m_CameraData = Data;

	Update();
}

void CCamera::Update()
{
	Rotate(0, 0, 0.0f);
	Move(EMovementDirection::Forward, 0.0f);
}
