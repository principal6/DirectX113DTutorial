#pragma once

#include "SharedHeader.h"

class CCamera
{
public:
	enum class EType
	{
		FirstPerson,
		ThirdPerson,
		FreeLook
	};

	enum class EMovementDirection
	{
		Forward,
		Backward,
		Rightward,
		Leftward,
	};

	struct SCameraData
	{
		SCameraData() {}
		SCameraData(EType _eType, XMVECTOR _EyePosition, XMVECTOR _FocusPosition, XMVECTOR _UpDirection = XMVectorSet(0, 1, 0, 0),
			float _ZoomDistance = KDefaultZoomDistance, float _MinZoomDistance = KDefaultMinZoomDistance, float _MaxZoomDistance = KDefaultMaxZoomDistance) :
			eType{ _eType }, EyePosition{ _EyePosition }, FocusPosition{ _FocusPosition },
			UpDirection{ _UpDirection }, BaseUpDirection{ _UpDirection }, BaseForwardDirection{ XMVector3Normalize(_FocusPosition - _EyePosition) },
			Forward{ BaseForwardDirection }, ZoomDistance{ _ZoomDistance }, MinZoomDistance{ _MinZoomDistance }, MaxZoomDistance{ _MaxZoomDistance } {}

		static constexpr float KDefaultZoomDistance{ 10.0f };
		static constexpr float KDefaultMinZoomDistance{ 1.0f };
		static constexpr float KDefaultMaxZoomDistance{ 50.0f };

		EType		eType{};
		XMVECTOR	EyePosition{ XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f) };
		XMVECTOR	FocusPosition{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };
		XMVECTOR	UpDirection{ XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };

		XMVECTOR	BaseForwardDirection{ XMVector3Normalize(FocusPosition - EyePosition) };
		XMVECTOR	BaseUpDirection{ UpDirection };

		float		ZoomDistance{ KDefaultZoomDistance };
		float		MinZoomDistance{ KDefaultMinZoomDistance };
		float		MaxZoomDistance{ KDefaultMaxZoomDistance };

		float		Pitch{};
		float		Yaw{};
		XMVECTOR	Forward{};

		float		MovementFactor{ 1.0f };

		XMMATRIX	WorldMatrix{};
	};

public:
	CCamera(const std::string Name) : m_Name{ Name } {}
	~CCamera() {}

public:
	void Move(EMovementDirection Direction, float DeltaTime);
	void Rotate(int DeltaX, int DeltaY, float DeltaTime);
	void Zoom(int DeltaWheel, float ZoomFactor = 1.0f);

	void SetEyePosition(const XMVECTOR& Position);
	void SetPitch(float Value);
	void SetYaw(float Value);
	void SetMovementFactor(float MovementFactor);

	void SetType(EType eType);
	EType GetType() const { return m_CameraData.eType; }

	void SetData(const SCameraData& Data);
	const SCameraData& GetData() const { return m_CameraData; }

	XMVECTOR& GetEyePosition() { return m_CameraData.EyePosition; }
	float& GetPitch() { return m_CameraData.Pitch; }
	float& GetYaw() { return m_CameraData.Yaw; }

	const XMVECTOR& GetEyePosition() const { return m_CameraData.EyePosition; }
	const XMVECTOR& GetFocusPosition() const { return m_CameraData.FocusPosition; }
	const XMVECTOR& GetUpDirection() const { return m_CameraData.UpDirection; }
	const XMVECTOR& GetForward() const { return m_CameraData.Forward; }
	float GetPitch() const { return m_CameraData.Pitch; }
	float GetYaw() const { return m_CameraData.Yaw; }
	float GetZoomDistance() const { return m_CameraData.ZoomDistance; }
	const std::string& GetName() const { return m_Name; }
	float GetMovementFactor() const { return m_CameraData.MovementFactor; }
	const XMMATRIX& GetWorldMatrix() const { return m_CameraData.WorldMatrix; }

public:
	void Update();

private:
	void UpdateWorldMatrix();

private:
	static constexpr float KPitchLimit{ XM_PIDIV2 - 0.01f };

private:
	std::string	m_Name{};
	SCameraData	m_CameraData{};
};