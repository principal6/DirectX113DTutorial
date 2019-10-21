#pragma once

#include "SharedHeader.h"

enum class ECameraType
{
	FirstPerson,
	ThirdPerson,
	FreeLook
};

enum class ECameraMovementDirection
{
	Forward,
	Backward,
	Rightward,
	Leftward,
};

struct SCameraData
{
	SCameraData() {}
	SCameraData(ECameraType _CameraType, XMVECTOR _EyePosition, XMVECTOR _FocusPosition, XMVECTOR _UpDirection = XMVectorSet(0, 1, 0, 0),
		float _Distance = KDefaultDistance, float _MinDistance = KDefaultMinDistance, float _MaxDistance = KDefaultMaxDistance) :
		CameraType{ _CameraType }, EyePosition{ _EyePosition }, FocusPosition{ _FocusPosition },
		UpDirection{ _UpDirection }, BaseUpDirection{ _UpDirection }, BaseForwardDirection{ XMVector3Normalize(_FocusPosition - _EyePosition) },
		Forward{ BaseForwardDirection }, ZoomDistance{ _Distance }, MinZoomDistance{ _MinDistance }, MaxZoomDistance{ _MaxDistance } {}

	static constexpr float KDefaultDistance{ 10.0f };
	static constexpr float KDefaultMinDistance{ 1.0f };
	static constexpr float KDefaultMaxDistance{ 50.0f };

	ECameraType	CameraType{};
	XMVECTOR	EyePosition{ XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f) };
	XMVECTOR	FocusPosition{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };
	XMVECTOR	UpDirection{ XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };

	XMVECTOR	BaseForwardDirection{};
	XMVECTOR	BaseUpDirection{};

	float		ZoomDistance{ KDefaultDistance };
	float		MinZoomDistance{ KDefaultMinDistance };
	float		MaxZoomDistance{ KDefaultMaxDistance };

	float		Pitch{};
	float		Yaw{};
	XMVECTOR	Forward{};
};

class CCamera
{
	friend class CGame;

public:
	CCamera(const SCameraData& CameraData) : m_CameraData{ CameraData } {}
	~CCamera() {}

public:
	void Move(ECameraMovementDirection Direction, float StrideFactor = 1.0f);
	void Rotate(int DeltaX, int DeltaY, float RotationFactor = 1.0f);
	void Zoom(int DeltaWheel, float ZoomFactor = 1.0f);

	void SetPosition(const XMVECTOR& Position);
	void SetPitch(float Value);
	void SetYaw(float Value);

	const SCameraData& GetData() { return m_CameraData; }

private:
	void Update();

private:
	static constexpr float KPitchLimit{ XM_PIDIV2 - 0.01f };

private:
	SCameraData m_CameraData{};
};