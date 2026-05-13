#include "Renderer.hpp"

#include <string>

#pragma comment(lib, "d3d12.lib") //links d3d12.lib
#pragma comment(lib, "dxgi.lib")

bool Renderer::Initialize(HWND window, UINT width, UINT height)
{
    m_window = window;
    m_width = width;
    m_height = height;

#if defined(_DEBUG)
    EnableDebugLayer();
#endif

    if (!CreateDXGI()) { return false; }

    if (!SelectAdapter()) { return false; }

    if (!CreateDevice()) { return false; }

    if (!CreateCommandQueue()) { return false; }

    if (!CreateSwapChain()) { return false; }

    if (!CreateRTVHeap()) { return false; }

    if (!CreateRTV()) { return false; }

    if (!CreateCommandAllocators()) { return false; }

    if (!CreateCommandList()) { return false; }

    if (!CreateFence()) { return false; }

    OutputDebugStringW(L"Renderer initialized.\n");
    return true;
}

void Renderer::Render()
{
    HRESULT result = m_commandAllocators[m_frameIndex]->Reset();

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to reset command allocator.\n");
        return;
    }

    result = m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr);

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to reset command list.\n");
        return;
    }

    D3D12_RESOURCE_BARRIER barrierRenderTarget = {};
    barrierRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierRenderTarget.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierRenderTarget.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrierRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrierRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_commandList->ResourceBarrier(1, &barrierRenderTarget);

    D3D12_CPU_DESCRIPTOR_HANDLE currentRTV = GetCurrentRTV();

    m_commandList->OMSetRenderTargets(1, &currentRTV, FALSE, nullptr);

    const float clearColor[] = { 0.0f, 0.5f, 0.5f, 1.0f };

    m_commandList->ClearRenderTargetView(currentRTV, clearColor, 0, nullptr);

    D3D12_RESOURCE_BARRIER barrierPresent = {};
    barrierPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierPresent.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrierPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrierPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_commandList->ResourceBarrier(1, &barrierPresent);

    m_commandList->Close();

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to close command list.\n");
        return;
    }

    ID3D12CommandList* commandLists[] = {
        m_commandList.Get()
    };

    m_commandQueue->ExecuteCommandLists(1, commandLists);

    result = m_swapChain->Present(1, 0);

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to present swap chain.\n");
        return;
    }

    WaitForGPU();

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
  
}

void Renderer::Shutdown()
{
    if (m_fenceEvent != nullptr)
    {
        CloseHandle(m_fenceEvent);
        m_fenceEvent = nullptr;
    }

    OutputDebugStringW(L"Renderer shutdown.\n");
}

#if defined(_DEBUG)
void Renderer::EnableDebugLayer()
{
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;

    HRESULT result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));

    if (SUCCEEDED(result))
    {
        debugController->EnableDebugLayer();
        OutputDebugStringW(L"D3D12 debug layer enabled.\n");
    }
    else
    {
        OutputDebugStringW(L"D3D12 debug layer not available.\n");
    }

}
#endif

bool Renderer::CreateDXGI()
{
    UINT factoryFlags = 0;

#if defined(_DEBUG)
    factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    HRESULT result = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory));

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to create DXGI factory.\n");
        return false;
    }

    OutputDebugStringW(L"DXGI factory created.\n");
    return true;
}

bool Renderer::SelectAdapter()
{
    UINT adapterIndex = 0;

    while (true) 
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

        HRESULT result = m_factory->EnumAdapters1(adapterIndex, adapter.GetAddressOf());

        if (result == DXGI_ERROR_NOT_FOUND) { break; }

        if (FAILED(result))
        {
            OutputDebugStringW(L"Failed while enumerating adapters.\n");
            return false;
        }

        DXGI_ADAPTER_DESC1 adapterDesc = {};
        adapter->GetDesc1(&adapterDesc);

        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapterIndex++;
            continue;
        }

        m_adapter = adapter;

        wchar_t message[256];
        swprintf_s(message, L"Selected adapter: %s\n", adapterDesc.Description);

        OutputDebugStringW(message);

        return true;
    }

    OutputDebugStringW(L"No suitable hardware adapter found.\n");
    return false;
}

bool Renderer::CreateDevice()
{
    HRESULT result = D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to create D3D12 device.\n");
        return false;
    }
    OutputDebugStringW(L"D3D12 device created.\n");
    return true;
}

bool Renderer::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    HRESULT result = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to create command queue.\n");
        return false;
    }
    OutputDebugStringW(L"Direct command queue created.\n");
    return true;
}

bool Renderer::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;

    Microsoft::WRL::ComPtr<IDXGISwapChain1> tempSwapChain;

    HRESULT result = m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_window, &swapChainDesc, nullptr, nullptr, &tempSwapChain);

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to create swapchain\n");
        return false;
    }

    result = tempSwapChain.As(&m_swapChain);

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to convert swap chain to IDXGISwapChain4.\n");
        return false;
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    m_factory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER);

    OutputDebugStringW(L"Swapchain created.\n");
    return true;
}

bool Renderer::CreateRTVHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.NumDescriptors = FrameCount;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;

    HRESULT result = m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvDescHeap));

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to create RTV descripter heap.\n");
        return false;
    }

    m_rtvDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    
    OutputDebugStringW(L"RTV descripter heap created.\n");
    return true;
}

bool Renderer::CreateRTV()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT i = 0; i < FrameCount; ++i)
    {
        HRESULT result = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));

        if (FAILED(result))
        {
            OutputDebugStringW(L"Failed to create swapchain backbuffer.\n");
            return false;
        }

        m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
        
        rtvHandle.ptr += m_rtvDescSize;
    }

    OutputDebugStringW(L"RTV created.\n");
    return true;
}

bool Renderer::CreateCommandAllocators()
{
    for (UINT i = 0; i < FrameCount; ++i)
    {
        HRESULT result = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]));

        if (FAILED(result))
        {
            OutputDebugStringW(L"Failed to create command allocator.\n");
            return false;
        }
    }

    OutputDebugStringW(L"Command allocators created.\n");
    return true;
}

bool Renderer::CreateCommandList()
{
    HRESULT result = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
        m_commandAllocators[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList));

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to create command list.\n");
        return false;
    }

    result = m_commandList->Close();

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to close command list.\n");
        return false;
    }

    OutputDebugStringW(L"Command list created.\n");
    return true;
}

bool Renderer::CreateFence()
{
    HRESULT result = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to create fence.\n");
        return false;
    }

    m_fenceValue = 1;
    m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    if (m_fenceEvent == nullptr)
    {
        OutputDebugStringW(L"Failed to create fence event.\n");
        return false;
    }

    OutputDebugStringW(L"Fence and Event created.\n");
    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::GetCurrentRTV() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart();

    rtvHandle.ptr += static_cast<SIZE_T>(m_frameIndex) * m_rtvDescSize;

    return rtvHandle;
}

void Renderer::WaitForGPU()
{
    const UINT64 fenceToWaitFor = m_fenceValue;

    HRESULT result = m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor);

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to signal fence.\n");
        return;
    }

    m_fenceValue++;

    if (m_fence->GetCompletedValue() < fenceToWaitFor)
    {
        result = m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);

        if (FAILED(result))
        {
            OutputDebugStringW(L"Failed to set fence event.\n");
            return;
        }

        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}
