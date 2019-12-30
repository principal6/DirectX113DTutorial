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
		XMMATRIX	ViewMatrix{};
	};

public:
	CCamera(const std::string Name) : m_Name{ Name } {}
	~CCamera() {}

public:
	void Move(EMovementDirection Direction, float DeltaTime);
	void Rotate(int dMouseX, int dMouseY, float DeltaTime);
	void Zoom(int DeltaWheel, float ZoomFactor = 1.0f);

	void Translate(const XMVECTOR& Delta);
	void TranslateTo(const XMVECTOR& Prime);
	void Rotate(float dPitch, float dYaw);

public:
	void SetType(EType eType);
	EType GetType() const { return m_CameraData.eType; }

	void SetData(const SCameraData& Data);
	const SCameraData& GetData() const { return m_CameraData; }

	void SetPitch(float Pitch);
	float GetPitch() const { return m_CameraData.Pitch; }

	void SetYaw(float Yaw);
	float GetYaw() const { return m_CameraData.Yaw; }

	void SetMovementFactor(float MovementFactor);
	float GetMovementFactor() const { return m_CameraData.MovementFactor; }

	void SetZoomDistance(float ZoomDistance);
	float GetZoomDistance() const;
	float GetZoomDistanceMin() const;
	float GetZoomDistanceMax() const;

public:
	const std::string& GetName() const { return m_Name; }
	const XMVECTOR& GetTranslation() const;
	const XMVECTOR& GetEyePosition() const;
	const XMVECTOR& GetForward() const { return m_CameraData.Forward; }
	const XMMATRIX& GetWorldMatrix() const { return m_CameraData.WorldMatrix; }
	const XMMATRIX& GetViewMatrix() const;

public:
	void Update();

private:
	void LimitRotationBoundary();
	void UpdateWorldMatrix();
	void UpdateViewMatrix();

private:
	static constexpr float KPitchLimit{ XM_PIDIV2 - 0.01f };

private:
	std::string	m_Name{};
	SCameraData	m_CameraData{};
};