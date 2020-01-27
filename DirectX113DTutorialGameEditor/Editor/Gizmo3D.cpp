#include "Gizmo3D.h"
#include "../Core/ConstantBuffer.h"
#include "../Core/PrimitiveGenerator.h"
#include "../Core/Shader.h"
#include "../Model/Object3D.h"

using std::make_unique;

CGizmo3D::CGizmo3D(ID3D11Device* const PtrDevice, ID3D11DeviceContext* const PtrDeviceContext) :
	m_PtrDevice{ PtrDevice }, m_PtrDeviceContext{ PtrDeviceContext }
{
	assert(m_PtrDevice);
	assert(m_PtrDeviceContext);
}

CGizmo3D::~CGizmo3D()
{
}

void CGizmo3D::Create()
{
	bool bShouldCompileShaders{ false };

	m_CBGizmoSpace = make_unique<CConstantBuffer>(m_PtrDevice, m_PtrDeviceContext, &m_CBGizmoSpaceData, sizeof(m_CBGizmoSpaceData));
	m_CBGizmoSpace->Create();

	m_CBGizmoColorFactor = make_unique<CConstantBuffer>(m_PtrDevice, m_PtrDeviceContext, &m_CBGizmoColorFactorData, sizeof(m_CBGizmoColorFactorData));
	m_CBGizmoColorFactor->Create();

	m_VSGizmo = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
	m_VSGizmo->Create(EShaderType::VertexShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\VSGizmo.hlsl", "main",
		CObject3D::KInputElementDescs, ARRAYSIZE(CObject3D::KInputElementDescs));
	m_VSGizmo->ReserveConstantBufferSlots(KVSSharedCBCount);
	m_VSGizmo->AttachConstantBuffer(m_CBGizmoSpace.get());

	m_PSGizmo = make_unique<CShader>(m_PtrDevice, m_PtrDeviceContext);
	m_PSGizmo->Create(EShaderType::PixelShader, CShader::EVersion::_4_0, bShouldCompileShaders, L"Shader\\PSGizmo.hlsl", "main");
	m_PSGizmo->ReserveConstantBufferSlots(KPSSharedCBCount);
	m_PSGizmo->AttachConstantBuffer(m_CBGizmoColorFactor.get());

	if (!m_RotationX)
	{
		m_RotationX = make_unique<CObject3D>("Gizmo", m_PtrDevice, m_PtrDeviceContext);
		SMesh MeshRing{ GenerateTorus(K3DGizmoRadius, 16, KRotationGizmoRingSegmentCount, KColorX) };
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorX) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_RotationX->Create(MergeStaticMeshes(MeshRing, MeshAxis));
		m_RotationX->RotateRollTo(-XM_PIDIV2);
	}

	if (!m_RotationY)
	{
		m_RotationY = make_unique<CObject3D>("Gizmo", m_PtrDevice, m_PtrDeviceContext);
		SMesh MeshRing{ GenerateTorus(K3DGizmoRadius, 16, KRotationGizmoRingSegmentCount, KColorY) };
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorY) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_RotationY->Create(MergeStaticMeshes(MeshRing, MeshAxis));
	}

	if (!m_RotationZ)
	{
		m_RotationZ = make_unique<CObject3D>("Gizmo", m_PtrDevice, m_PtrDeviceContext);
		SMesh MeshRing{ GenerateTorus(K3DGizmoRadius, 16, KRotationGizmoRingSegmentCount, KColorZ) };
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorZ) };
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_RotationZ->Create(MergeStaticMeshes(MeshRing, MeshAxis));
		m_RotationZ->RotatePitchTo(XM_PIDIV2);
	}

	if (!m_TranslationX)
	{
		m_TranslationX = make_unique<CObject3D>("Gizmo", m_PtrDevice, m_PtrDeviceContext);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorX) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, KColorX) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_TranslationX->Create(MeshAxis);
		m_TranslationX->RotateRollTo(-XM_PIDIV2);
	}

	if (!m_TranslationY)
	{
		m_TranslationY = make_unique<CObject3D>("Gizmo", m_PtrDevice, m_PtrDeviceContext);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorY) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, KColorY) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_TranslationY->Create(MeshAxis);
	}

	if (!m_TranslationZ)
	{
		m_TranslationZ = make_unique<CObject3D>("Gizmo", m_PtrDevice, m_PtrDeviceContext);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorZ) };
		SMesh MeshCone{ GenerateCone(0, 0.1f, 0.5f, 16, KColorZ) };
		TranslateMesh(MeshCone, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCone);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_TranslationZ->Create(MeshAxis);
		m_TranslationZ->RotatePitchTo(XM_PIDIV2);
	}

	if (!m_ScalingX)
	{
		m_ScalingX = make_unique<CObject3D>("Gizmo", m_PtrDevice, m_PtrDeviceContext);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorX) };
		SMesh MeshCube{ GenerateCube(KColorX) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_ScalingX->Create(MeshAxis);
		m_ScalingX->RotateRollTo(-XM_PIDIV2);
	}

	if (!m_ScalingY)
	{
		m_ScalingY = make_unique<CObject3D>("Gizmo", m_PtrDevice, m_PtrDeviceContext);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorY) };
		SMesh MeshCube{ GenerateCube(KColorY) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_ScalingY->Create(MeshAxis);
	}

	if (!m_ScalingZ)
	{
		m_ScalingZ = make_unique<CObject3D>("Gizmo", m_PtrDevice, m_PtrDeviceContext);
		SMesh MeshAxis{ GenerateCylinder(K3DGizmoRadius, 1.0f, 16, KColorZ) };
		SMesh MeshCube{ GenerateCube(KColorZ) };
		ScaleMesh(MeshCube, XMVectorSet(0.2f, 0.2f, 0.2f, 0));
		TranslateMesh(MeshCube, XMVectorSet(0, 0.5f, 0, 0));
		MeshAxis = MergeStaticMeshes(MeshAxis, MeshCube);
		TranslateMesh(MeshAxis, XMVectorSet(0, 0.5f, 0, 0));
		m_ScalingZ->Create(MeshAxis);
		m_ScalingZ->RotatePitchTo(XM_PIDIV2);
	}
}

void CGizmo3D::CaptureTranslation(const XMVECTOR& Translation)
{
	m_GizmoTranslation = Translation / XMVectorGetW(Translation);
}

void CGizmo3D::UpdateTranslation(const XMVECTOR& CameraPosition)
{
	// @important
	// Translate gizmos
	m_TranslationX->TranslateTo(m_GizmoTranslation);
	m_TranslationY->TranslateTo(m_GizmoTranslation);
	m_TranslationZ->TranslateTo(m_GizmoTranslation);
	m_RotationX->TranslateTo(m_GizmoTranslation);
	m_RotationY->TranslateTo(m_GizmoTranslation);
	m_RotationZ->TranslateTo(m_GizmoTranslation);
	m_ScalingX->TranslateTo(m_GizmoTranslation);
	m_ScalingY->TranslateTo(m_GizmoTranslation);
	m_ScalingZ->TranslateTo(m_GizmoTranslation);

	// @important
	// Calculate scalar IAW the distance from the camera	
	m_GizmoDistanceScalar = XMVectorGetX(XMVector3Length(CameraPosition - m_GizmoTranslation)) * 0.1f;
	m_GizmoDistanceScalar = pow(m_GizmoDistanceScalar, 0.7f);
}

void CGizmo3D::SetMode(EMode eMode)
{
	m_eCurrentMode = eMode;
}

bool CGizmo3D::Interact(const XMVECTOR& PickingRayOrigin, const XMVECTOR& PickingRayDirection, 
	const XMVECTOR& CameraPosition, const XMVECTOR& CameraForward, int MouseX, int MouseY, bool bMouseLeftDown)
{
	bool bResult{ false };
	if (IsInAction())
	{
		float DotXAxisForward{ XMVectorGetX(XMVector3Dot(XMVectorSet(1, 0, 0, 0), CameraForward)) };
		float DotZAxisForward{ XMVectorGetX(XMVector3Dot(XMVectorSet(0, 0, 1, 0), CameraForward)) };

		int DeltaX{ MouseX - m_CapturedMouseX };
		int DeltaY{ MouseY - m_CapturedMouseY };

		int Delta{};
		if (m_eCurrentMode == EMode::Rotation)
		{
			Delta = (m_eSelectedAxis == EAxis::AxisY) ? -DeltaX : -DeltaY;
		}
		else // Translation & Scaling
		{
			switch (m_eSelectedAxis)
			{
			default:
			case EAxis::None:
				break;
			case EAxis::AxisX:
				if (DotZAxisForward < 0.0f) DeltaX = -DeltaX;
				Delta = DeltaX;
				break;
			case EAxis::AxisY:
				Delta = -DeltaY;
				break;
			case EAxis::AxisZ:
				if (DotXAxisForward >= 0.0f) DeltaX = -DeltaX;
				Delta = DeltaX;
				break;
			}
		}
		int DeltaSign{};
		if (Delta != 0)
		{
			if (Delta > +1) DeltaSign = +1;
			if (Delta < -1) DeltaSign = -1;
		}
		if (DeltaSign != 0)
		{
			float DistanceObejctCamera{ XMVectorGetX(XMVector3Length(m_GizmoTranslation - CameraPosition)) };
			float DeltaFactor{ KMovementFactorBase };
			if (DistanceObejctCamera > K3DGizmoCameraDistanceThreshold4) DeltaFactor *= 32.0f; // 2.0f
			else if (DistanceObejctCamera > K3DGizmoCameraDistanceThreshold3) DeltaFactor *= 16.0f; // 1.0f
			else if (DistanceObejctCamera > K3DGizmoCameraDistanceThreshold2) DeltaFactor *= 8.0f; // 0.5f
			else if (DistanceObejctCamera > K3DGizmoCameraDistanceThreshold1) DeltaFactor *= 4.0f; // 0.25f
			else if (DistanceObejctCamera > K3DGizmoCameraDistanceThreshold0) DeltaFactor *= 2.0f; // 0.125f

			float DeltaTranslationScalar{ DeltaSign * DeltaFactor };
			float DeltaRotationScalar{ DeltaSign * KRotation360To2PI * KRotationDelta };
			float DeltaScalingScalar{ DeltaSign * DeltaFactor };
			switch (m_eCurrentMode)
			{
			case EMode::Translation:
				DeltaRotationScalar = 0;
				DeltaScalingScalar = 0;
				break;
			case EMode::Rotation:
				DeltaTranslationScalar = 0;
				DeltaScalingScalar = 0;
				break;
			case EMode::Scaling:
				DeltaTranslationScalar = 0;
				DeltaRotationScalar = 0;
				break;
			default:
				break;
			}

			m_DeltaTranslation = XMVectorSet(
				(m_eSelectedAxis == EAxis::AxisX) ? DeltaTranslationScalar : 0,
				(m_eSelectedAxis == EAxis::AxisY) ? DeltaTranslationScalar : 0,
				(m_eSelectedAxis == EAxis::AxisZ) ? DeltaTranslationScalar : 0, 0);
			m_DeltaPitch = (m_eSelectedAxis == EAxis::AxisX) ? DeltaRotationScalar : 0;
			m_DeltaYaw = (m_eSelectedAxis == EAxis::AxisY) ? DeltaRotationScalar : 0;
			m_DeltaRoll = (m_eSelectedAxis == EAxis::AxisZ) ? DeltaRotationScalar : 0;
			m_DeltaScaling = XMVectorSet(
				(m_eSelectedAxis == EAxis::AxisX) ? DeltaScalingScalar : 0,
				(m_eSelectedAxis == EAxis::AxisY) ? DeltaScalingScalar : 0,
				(m_eSelectedAxis == EAxis::AxisZ) ? DeltaScalingScalar : 0, 0);

			m_CapturedMouseX = MouseX;
			m_CapturedMouseY = MouseY;
			m_GizmoTranslation += m_DeltaTranslation;

			bResult = true;
		}
	}
	else
	{
		m_CapturedMouseX = MouseX;
		m_CapturedMouseY = MouseY;

		m_PickingRayOrigin = PickingRayOrigin;
		m_PickingRayDirection = PickingRayDirection;

		switch (m_eCurrentMode)
		{
		case EMode::Translation:
			m_bIsHovered = true;
			if (IsInteractingTS(EAxis::AxisX))
			{
				m_eSelectedAxis = EAxis::AxisX;
			}
			else if (IsInteractingTS(EAxis::AxisY))
			{
				m_eSelectedAxis = EAxis::AxisY;
			}
			else if (IsInteractingTS(EAxis::AxisZ))
			{
				m_eSelectedAxis = EAxis::AxisZ;
			}
			else
			{
				m_bIsHovered = false;
			}
			break;
		case EMode::Rotation:
		{
			XMVECTOR T{ KVectorGreatest };
			m_bIsHovered = false;
			if (IsInteractingR(EAxis::AxisX, &T))
			{
				m_eSelectedAxis = EAxis::AxisX;
				m_bIsHovered = true;
			}
			if (IsInteractingR(EAxis::AxisY, &T))
			{
				m_eSelectedAxis = EAxis::AxisY;
				m_bIsHovered = true;
			}
			if (IsInteractingR(EAxis::AxisZ, &T))
			{
				m_eSelectedAxis = EAxis::AxisZ;
				m_bIsHovered = true;
			}
			break;
		}
		case EMode::Scaling:
			m_bIsHovered = true;
			if (IsInteractingTS(EAxis::AxisX))
			{
				m_eSelectedAxis = EAxis::AxisX;
			}
			else if (IsInteractingTS(EAxis::AxisY))
			{
				m_eSelectedAxis = EAxis::AxisY;
			}
			else if (IsInteractingTS(EAxis::AxisZ))
			{
				m_eSelectedAxis = EAxis::AxisZ;
			}
			else
			{
				m_bIsHovered = false;
			}
			break;
		default:
			break;
		}

		if (m_bIsHovered && bMouseLeftDown)
		{
			m_bIsInAction = true;
		}
		else if (!m_bIsHovered)
		{
			m_eSelectedAxis = EAxis::None;
		}
	}

	UpdateTranslation(CameraPosition);
	
	return bResult;
}

void CGizmo3D::QuitAction()
{
	m_bIsInAction = false;
}

bool CGizmo3D::IsInteractingTS(EAxis eAxis)
{
	static constexpr float KGizmoLengthFactor{ 1.1875f };
	static constexpr float KGizmoRaidus{ 0.05859375f };

	XMVECTOR CylinderSpaceRayOrigin{ m_PickingRayOrigin - m_GizmoTranslation };
	XMVECTOR CylinderSpaceRayDirection{ m_PickingRayDirection };

	switch (eAxis)
	{
	case EAxis::None:
		return false;
		break;
	case EAxis::AxisX:
	{
		XMMATRIX RotationMatrix{ XMMatrixRotationZ(XM_PIDIV2) };
		CylinderSpaceRayOrigin = XMVector3TransformCoord(CylinderSpaceRayOrigin, RotationMatrix);
		CylinderSpaceRayDirection = XMVector3TransformNormal(CylinderSpaceRayDirection, RotationMatrix);
		if (IntersectRayCylinder(CylinderSpaceRayOrigin, CylinderSpaceRayDirection,
			KGizmoLengthFactor * m_GizmoDistanceScalar, KGizmoRaidus * m_GizmoDistanceScalar)) return true;
	}
	break;
	case EAxis::AxisY:
		if (IntersectRayCylinder(CylinderSpaceRayOrigin, CylinderSpaceRayDirection,
			KGizmoLengthFactor * m_GizmoDistanceScalar, KGizmoRaidus * m_GizmoDistanceScalar)) return true;
		break;
	case EAxis::AxisZ:
	{
		XMMATRIX RotationMatrix{ XMMatrixRotationX(-XM_PIDIV2) };
		CylinderSpaceRayOrigin = XMVector3TransformCoord(CylinderSpaceRayOrigin, RotationMatrix);
		CylinderSpaceRayDirection = XMVector3TransformNormal(CylinderSpaceRayDirection, RotationMatrix);
		if (IntersectRayCylinder(CylinderSpaceRayOrigin, CylinderSpaceRayDirection,
			KGizmoLengthFactor * m_GizmoDistanceScalar, KGizmoRaidus * m_GizmoDistanceScalar)) return true;
	}
	break;
	default:
		break;
	}
	return false;
}

bool CGizmo3D::IsInteractingR(EAxis eAxis, XMVECTOR* const OutPtrT)
{
	static constexpr float KHollowCylinderInnerRaidus{ 0.9375f };
	static constexpr float KHollowCylinderOuterRaidus{ 1.0625f };
	static constexpr float KHollowCylinderHeight{ 0.125f };

	XMVECTOR CylinderSpaceRayOrigin{ m_PickingRayOrigin - m_GizmoTranslation };
	XMVECTOR CylinderSpaceRayDirection{ m_PickingRayDirection };

	XMVECTOR NewT{ KVectorGreatest };
	switch (eAxis)
	{
	case EAxis::None:
	{
		return false;
	}
	case EAxis::AxisX:
	{
		XMMATRIX RotationMatrix{ XMMatrixRotationZ(XM_PIDIV2) };
		CylinderSpaceRayOrigin = XMVector3TransformCoord(CylinderSpaceRayOrigin, RotationMatrix);
		CylinderSpaceRayDirection = XMVector3TransformNormal(CylinderSpaceRayDirection, RotationMatrix);
		if (IntersectRayHollowCylinderCentered(CylinderSpaceRayOrigin, CylinderSpaceRayDirection, KHollowCylinderHeight,
			KHollowCylinderInnerRaidus * m_GizmoDistanceScalar, KHollowCylinderOuterRaidus * m_GizmoDistanceScalar, &NewT))
		{
			if (XMVector3Less(NewT, *OutPtrT))
			{
				*OutPtrT = NewT;
				return true;
			}
		}
		break;
	}
	case EAxis::AxisY:
	{
		if (IntersectRayHollowCylinderCentered(CylinderSpaceRayOrigin, CylinderSpaceRayDirection, KHollowCylinderHeight,
			KHollowCylinderInnerRaidus * m_GizmoDistanceScalar, KHollowCylinderOuterRaidus * m_GizmoDistanceScalar, &NewT))
		{
			if (XMVector3Less(NewT, *OutPtrT))
			{
				*OutPtrT = NewT;
				return true;
			}
		}
		break;
	}
	case EAxis::AxisZ:
	{
		XMMATRIX RotationMatrix{ XMMatrixRotationX(XM_PIDIV2) };
		CylinderSpaceRayOrigin = XMVector3TransformCoord(CylinderSpaceRayOrigin, RotationMatrix);
		CylinderSpaceRayDirection = XMVector3TransformNormal(CylinderSpaceRayDirection, RotationMatrix);
		if (IntersectRayHollowCylinderCentered(CylinderSpaceRayOrigin, CylinderSpaceRayDirection, KHollowCylinderHeight,
			KHollowCylinderInnerRaidus * m_GizmoDistanceScalar, KHollowCylinderOuterRaidus * m_GizmoDistanceScalar, &NewT))
		{
			if (XMVector3Less(NewT, *OutPtrT))
			{
				*OutPtrT = NewT;
				return true;
			}
		}
		break;
	}
	default:
		break;
	}
	return false;
}

void CGizmo3D::Draw(const XMMATRIX& ViewProjection)
{
	m_VSGizmo->Use();
	m_PSGizmo->Use();

	bool bShouldHighlightX{ m_eSelectedAxis == EAxis::AxisX };
	bool bShouldHighlightY{ m_eSelectedAxis == EAxis::AxisY };
	bool bShouldHighlightZ{ m_eSelectedAxis == EAxis::AxisZ };
	switch (m_eCurrentMode)
	{
	case CGizmo3D::EMode::Translation:
		DrawGizmo(m_TranslationX.get(), ViewProjection, bShouldHighlightX);
		DrawGizmo(m_TranslationY.get(), ViewProjection, bShouldHighlightY);
		DrawGizmo(m_TranslationZ.get(), ViewProjection, bShouldHighlightZ);
		break;
	case CGizmo3D::EMode::Rotation:
		DrawGizmo(m_RotationX.get(), ViewProjection, bShouldHighlightX);
		DrawGizmo(m_RotationY.get(), ViewProjection, bShouldHighlightY);
		DrawGizmo(m_RotationZ.get(), ViewProjection, bShouldHighlightZ);
		break;
	case CGizmo3D::EMode::Scaling:
		DrawGizmo(m_ScalingX.get(), ViewProjection, bShouldHighlightX);
		DrawGizmo(m_ScalingY.get(), ViewProjection, bShouldHighlightY);
		DrawGizmo(m_ScalingZ.get(), ViewProjection, bShouldHighlightZ);
		break;
	default:
		break;
	}
}

void CGizmo3D::DrawGizmo(CObject3D* const Gizmo, const XMMATRIX& ViewProjection, bool bShouldHighlight)
{
	Gizmo->ScaleTo(XMVectorSet(m_GizmoDistanceScalar, m_GizmoDistanceScalar, m_GizmoDistanceScalar, 0.0f));
	Gizmo->UpdateWorldMatrix();

	m_CBGizmoSpaceData.WVP = XMMatrixTranspose(Gizmo->GetWorldMatrix() * ViewProjection);
	m_CBGizmoSpace->Update();

	// @important: alpha 0 means color overriding in PS
	m_CBGizmoColorFactorData.ColorFactor = (bShouldHighlight) ? XMVectorSet(1.0f, 1.0f, 0.0f, 0.0f) : XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	m_CBGizmoColorFactor->Update();

	Gizmo->Draw();
}

const XMVECTOR& CGizmo3D::GetDeltaTranslation() const
{
	return m_DeltaTranslation;
}

float CGizmo3D::GetDeltaPitch() const
{
	return m_DeltaPitch;
}

float CGizmo3D::GetDeltaYaw() const
{
	return m_DeltaYaw;
}

float CGizmo3D::GetDeltaRoll() const
{
	return m_DeltaRoll;
}

const XMVECTOR& CGizmo3D::GetDeltaScaling() const
{
	return m_DeltaScaling;
}

bool CGizmo3D::IsInAction() const
{
	return m_bIsInAction;
}
