#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>

#include "TrianglePass.hpp"

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
	bool CreateSwapChain();
	bool CreateRTVHeap();
	bool CreateRTV();
	bool CreateCommandAllocators();
	bool CreateCommandList();
	bool CreateFence();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;
	void WaitForGPU();

private:
	static constexpr UINT FrameCount = 2;

	HWND m_window = nullptr;
	UINT m_width = 0;
	UINT m_height = 0;
	UINT m_frameIndex = 0;
	UINT m_rtvDescSize = 0;
	UINT m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvDescHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
};