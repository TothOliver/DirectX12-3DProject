#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <chrono>
#include "MeshPass.hpp"

struct FrameResource
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
	UINT64 FenceValue = 0;
};

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
	bool CreateDepthBuffer();

	bool CreateCommandAllocators();
	bool CreateCommandList();
	bool CreateFence();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRTV() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentDSV() const;

	void WaitForGPU();
	bool WaitForFrameResource(FrameResource& frameResource);

private:
	static constexpr UINT FrameCount = 2;
	static constexpr DXGI_FORMAT DepthFormat = DXGI_FORMAT_D32_FLOAT;

	using Clock = std::chrono::steady_clock;

	Clock::time_point m_previousTime;
	float m_deltaTime = 0.0f;

	HWND m_window = nullptr;
	UINT m_width = 0;
	UINT m_height = 0;
	UINT m_frameIndex = 0;
	UINT m_currentFrameResourceIndex = 0;
	UINT m_rtvDescSize = 0;


	Microsoft::WRL::ComPtr<IDXGIFactory6> m_factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> m_adapter;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvDescHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvDescHeap;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

	FrameResource m_frameResources[FrameCount];
	UINT m_nextFenceValue = 1;
	HANDLE m_fenceEvent = nullptr;

	MeshPass m_meshPass;
};