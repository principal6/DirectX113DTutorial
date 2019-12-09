#include "Camera.h"

using std::max;
using std::min;

void CCamera::Move(EMovementDirection Direction, float DeltaTime)
{
	DeltaTime *= m_CameraData.MovementFactor;

	XMVECTOR dPosition{};
	if (m_CameraData.eType == EType::FreeLook)
	{
		XMVECTOR Rightward{ XMVector3Normalize(XMVector3Cross(m_CameraData.UpDirection, m_CameraData.Forward)) };

		switch (Direction)
		{
		case EMovementDirection::Forward:
			dPosition = +m_CameraData.Forward * DeltaTime;
			break;
		case EMovementDirection::Backward:
			dPosition = -m_CameraData.Forward * DeltaTime;
			break;
		case EMovementDirection::Rightward:
			dPosition = +Rightward * DeltaTime;
			break;
		case EMovementDirection::Leftward:
			dPosition = -Rightward * DeltaTime;
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
			dPosition = +GroundForward * DeltaTime;
			break;
		case EMovementDirection::Backward:
			dPosition = -GroundForward * DeltaTime;
			break;
		case EMovementDirection::Rightward:
			dPosition = +GroundRightward * DeltaTime;
			break;
		case EMovementDirection::Leftward:
			dPosition = -GroundRightward * DeltaTime;
			break;
		default:
			break;
		}
	}

	m_CameraData.EyePosition += dPosition;
	m_CameraData.FocusPosition += dPosition;

	Update();
}

void CCamera::Rotate(int dMouseX, int dMouseY, float DeltaTime)
{
	m_CameraData.Pitch += DeltaTime * dMouseY;
	m_CameraData.Yaw += DeltaTime * dMouseX;

	LimitRotationBoundary();

	Update();
}

void CCamera::Zoom(int DeltaWheel, float ZoomFactor)
{
	if (m_CameraData.eType != EType::ThirdPerson) return;

	m_CameraData.ZoomDistance -= DeltaWheel * ZoomFactor;
	m_CameraData.ZoomDistance = max(m_CameraData.ZoomDistance, m_CameraData.MinZoomDistance);
	m_CameraData.ZoomDistance = min(m_CameraData.ZoomDistance, m_CameraData.MaxZoomDistance);

	Update();
}

void CCamera::Translate(const XMVECTOR& Delta)
{
	if (m_CameraData.eType == EType::ThirdPerson)
	{
		m_CameraData.FocusPosition += Delta;
	}
	else
	{
		m_CameraData.EyePosition += Delta;
	}

	Update();
}

void CCamera::TranslateTo(const XMVECTOR& Prime)
{
	if (m_CameraData.eType == EType::ThirdPerson)
	{
		m_CameraData.FocusPosition = Prime;
	}
	else
	{
		m_CameraData.EyePosition = Prime;
	}

	Update();
}

void CCamera::Rotate(float dPitch, float dYaw)
{
	m_CameraData.Pitch += dPitch;
	m_CameraData.Yaw += dYaw;

	LimitRotationBoundary();

	Update();
}

void CCamera::SetPitch(float Pitch)
{
	m_CameraData.Pitch = Pitch;

	LimitRotationBoundary();

	Update();
}

void CCamera::SetYaw(float Yaw)
{
	m_CameraData.Yaw = Yaw;

	LimitRotationBoundary();

	Update();
}

void CCamera::SetMovementFactor(float MovementFactor)
{
	m_CameraData.MovementFactor = MovementFactor;
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

const XMVECTOR& CCamera::GetTranslation() const
{
	if (m_CameraData.eType == EType::FirstPerson || m_CameraData.eType == EType::FreeLook)
	{
		return m_CameraData.EyePosition;
	}
	else
	{
		return m_CameraData.FocusPosition;
	}
}

const XMVECTOR& CCamera::GetEyePosition() const
{
	return m_CameraData.EyePosition;
}

const XMMATRIX& CCamera::GetViewMatrix() const
{
	return m_CameraData.ViewMatrix;
}

void CCamera::Update()
{
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
	
	UpdateWorldMatrix();
	UpdateViewMatrix();
}

void CCamera::LimitRotationBoundary()
{
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
}

void CCamera::UpdateWorldMatrix()
{
	XMMATRIX Translation{ XMMatrixTranslationFromVector(m_CameraData.EyePosition) };
	XMMATRIX Rotation{ XMMatrixRotationRollPitchYaw(m_CameraData.Pitch, m_CameraData.Yaw, 0) };

	m_CameraData.WorldMatrix = Rotation * Translation;
}

void CCamera::UpdateViewMatrix()
{
	m_CameraData.ViewMatrix = XMMatrixLookAtLH(m_CameraData.EyePosition, m_CameraData.FocusPosition, m_CameraData.UpDirection);
}
