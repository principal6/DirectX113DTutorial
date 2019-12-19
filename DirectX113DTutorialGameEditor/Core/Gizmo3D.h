#pragma once

#include "SharedHeader.h"

class CShader;
class CConstantBuffer;
class CObject3D;

class CGizmo3D
{
public:
	enum class EMode
	{
		Translation,
		Rotation,
		Scaling
	};

	enum class EAxis
	{
		None,
		AxisX,
		AxisY,
		AxisZ
	};

private:
	struct SCBGizmoSpaceData
	{
		XMMATRIX	WVP{};
	};

	struct SCBGizmoColorFactorData
	{
		XMVECTOR	ColorFactor{};
	};

public:
	CGizmo3D(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext);
	~CGizmo3D();

public:
	void Create();

public:
	void CaptureTranslation(const XMVECTOR& Translation);
	void UpdateTranslation(const XMVECTOR& CameraPosition);
	void SetMode(EMode eMode);

public:
	bool Interact(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection,
		const XMVECTOR& CameraPosition, const XMVECTOR& CameraForward, int MouseX, int MouseY, bool bMouseLeftDown);

	void QuitAction();

private:
	bool IsInteractingTS(EAxis eAxis);
	bool IsInteractingR(EAxis eAxis, XMVECTOR* const OutPtrT);

public:
	void Draw(const XMMATRIX& ViewProjection);

private:
	void DrawGizmo(CObject3D* const Gizmo, const XMMATRIX& ViewProjection, bool bShouldHighlight);

public:
	const XMVECTOR& GetDeltaTranslation() const;
	float GetDeltaPitch() const;
	float GetDeltaYaw() const;
	float GetDeltaRoll() const;
	const XMVECTOR& GetDeltaScaling() const;
	bool IsInAction() const;

public:
	static constexpr XMVECTOR KColorX{ 1.000f, 0.125f, 0.125f, 1.0f };
	static constexpr XMVECTOR KColorY{ 0.125f, 1.000f, 0.125f, 1.0f };
	static constexpr XMVECTOR KColorZ{ 0.125f, 0.125f, 1.000f, 1.0f };
	static constexpr float KMovementFactorBase{ 0.0625f };
	static constexpr float KRotation360To2PI{ 1.0f / 360.0f * XM_2PI };
	static constexpr float KRotation2PITo360{ 1.0f / XM_2PI * 360.0f };
	static constexpr float KRotationDelta{ 22.5f };
	static constexpr float K3DGizmoRadius{ 0.05f };
	static constexpr float K3DGizmoSelectionRadius{ 1.1f };
	static constexpr float K3DGizmoSelectionLowBoundary{ 0.8f };
	static constexpr float K3DGizmoSelectionHighBoundary{ 1.2f };
	static constexpr float K3DGizmoCameraDistanceThreshold0{ 2.0f };
	static constexpr float K3DGizmoCameraDistanceThreshold1{ 4.0f };
	static constexpr float K3DGizmoCameraDistanceThreshold2{ 8.0f };
	static constexpr float K3DGizmoCameraDistanceThreshold3{ 16.0f };
	static constexpr float K3DGizmoCameraDistanceThreshold4{ 32.0f };
	static constexpr float K3DGizmoDistanceFactorExponent{ 0.75f };
	static constexpr int KRotationGizmoRingSegmentCount{ 36 };

private:
	ID3D11Device* const					m_PtrDevice{};
	ID3D11DeviceContext* const			m_PtrDeviceContext{};

private:
	EMode								m_eCurrentMode{};
	EAxis								m_eSelectedAxis{};
	XMVECTOR							m_GizmoTranslation{};
	XMVECTOR							m_PickingRayOrigin{};
	XMVECTOR							m_PickingRayDirection{};
	float								m_GizmoDistanceScalar{ 1.0f };
	bool								m_bIsHovered{};
	bool								m_bIsInAction{};
	int									m_CapturedMouseX{};
	int									m_CapturedMouseY{};

private:
	XMVECTOR							m_DeltaTranslation{};
	float								m_DeltaPitch{};
	float								m_DeltaYaw{};
	float								m_DeltaRoll{};
	XMVECTOR							m_DeltaScaling{};

private:
	std::unique_ptr<CConstantBuffer>	m_CBGizmoSpace{};
	SCBGizmoSpaceData					m_CBGizmoSpaceData{};

	std::unique_ptr<CConstantBuffer>	m_CBGizmoColorFactor{};
	SCBGizmoColorFactorData				m_CBGizmoColorFactorData{};

	std::unique_ptr<CShader>			m_VSGizmo{};
	std::unique_ptr<CShader>			m_PSGizmo{};

private:
	std::unique_ptr<CObject3D>			m_TranslationX{};
	std::unique_ptr<CObject3D>			m_TranslationY{};
	std::unique_ptr<CObject3D>			m_TranslationZ{};

	std::unique_ptr<CObject3D>			m_RotationX{}; // Pitch
	std::unique_ptr<CObject3D>			m_RotationY{}; // Yaw
	std::unique_ptr<CObject3D>			m_RotationZ{}; // Roll

	std::unique_ptr<CObject3D>			m_ScalingX{};
	std::unique_ptr<CObject3D>			m_ScalingY{};
	std::unique_ptr<CObject3D>			m_ScalingZ{};
};
