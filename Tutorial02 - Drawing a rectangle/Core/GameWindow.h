#pragma once

#include <Windows.h>
#include "SharedHeader.h"

class CGameWindow
{
public:
	CGameWindow(HINSTANCE hInstance, const XMFLOAT2& WindowSize) : m_hInstance{ hInstance }, m_WindowSize{ WindowSize } {}
	~CGameWindow() {}

	void CreateWin32(WNDPROC WndProc, LPCTSTR WindowName, bool bWindowed);

	void BeginRendering(const FLOAT* ClearColor);
	void EndRendering();

public:
	ID3D11Device* GetDevicePtr() { return m_Device.Get(); }
	ID3D11DeviceContext* GetDeviceContextPtr() { return m_DeviceContext.Get(); }

private:
	void CreateWin32Window(WNDPROC WndProc, LPCTSTR WindowName);
	void InitializeDirectX(bool bWindowed);

private:
	void CreateSwapChain(bool bWindowed);
	void CreateSetViews();
	void SetViewports();

private:
	HWND		m_hWnd{};
	HINSTANCE	m_hInstance{};
	XMFLOAT2	m_WindowSize{};

private:
	ComPtr<IDXGISwapChain>			m_SwapChain{};
	ComPtr<ID3D11Device>			m_Device{};
	ComPtr<ID3D11DeviceContext>		m_DeviceContext{};
	ComPtr<ID3D11RenderTargetView>	m_RenderTargetView{};
	ComPtr<ID3D11DepthStencilView>	m_DepthStencilView{};
	ComPtr<ID3D11Texture2D>			m_DepthStencilBuffer{};
};