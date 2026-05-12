#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>


class Renderer 
{
public:
	bool Initialize(HWND window, UINT width, UINT height);
	void Render();
	void Shutdown();

private:
#if defined(_DEBUG)
	void EnableDebugLayer();
#endif
	bool CreateDXGI();
	bool SelectAdapter();
	bool CreateDevice();
	bool CreateCommandQueue();

private:
	HWND m_window = nullptr;
	UINT m_width = 0;
	UINT m_height = 0;

	Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
};