#include "Renderer.hpp"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#pragma comment(lib, "d3dcompiler.lib")
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

    if (!CreateDeviceDependantResources()) { return false; }

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

    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    m_commandList->SetGraphicsRootSignature(
        m_rootSignature.Get()
    );

    m_commandList->SetPipelineState(
        m_pipelineState.Get()
    );

    m_commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    m_commandList->IASetVertexBuffers(
        0,
        1,
        &m_vertexBufferView
    );

    m_commandList->DrawInstanced(3, 1, 0, 0);
    //Trianglepass here with draw call?

    D3D12_RESOURCE_BARRIER barrierPresent = {};
    barrierPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierPresent.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    barrierPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrierPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    m_commandList->ResourceBarrier(1, &barrierPresent);

    result = m_commandList->Close();

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

bool Renderer::CreateDeviceDependantResources()
{
    if (!CreateRootSignature()) { return false; }

    if (!CreateShaders()) { return false; }

    if (!CreatePipelineState()) { return false; }

    if (!CreateVertexBuffer()) { return false; }

    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = static_cast<float>(m_width);
    m_viewport.Height = static_cast<float>(m_height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(m_width);
    m_scissorRect.bottom = static_cast<LONG>(m_height);

    return true;
}

bool Renderer::CreateRootSignature()
{
    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signature;
    Microsoft::WRL::ComPtr<ID3DBlob> error;

    HRESULT result = D3D12SerializeRootSignature(
        &desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature,
        &error
    );

    if (FAILED(result))
    {
        return false;
    }

    result = m_device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    return SUCCEEDED(result);
}

bool Renderer::CreateShaders()
{
    UINT compileFlags = 0;

#if defined(_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT result = D3DCompileFromFile(
        L"VertexShader.hlsl",
        nullptr,
        nullptr,
        "VSMain",
        "vs_5_0",
        compileFlags,
        0,
        &m_vertexShader,
        nullptr
    );

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to compile vertex shader.\n");
        return false;
    }

    result = D3DCompileFromFile(
        L"PixelShader.hlsl",
        nullptr,
        nullptr,
        "PSMain",
        "ps_5_0",
        compileFlags,
        0,
        &m_pixelShader,
        nullptr
    );

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to compile pixel shader.\n");
        return false;
    }

    return true;
}

bool Renderer::CreatePipelineState()
{
    static const D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        {
            "POSITION",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            0,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "COLOR",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            0,
            12,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        }
    };

    D3D12_RASTERIZER_DESC rasterizerDesc = {};

    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizerDesc.FrontCounterClockwise = FALSE;

    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias =
        D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster =
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blendDesc = {};

    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.IndependentBlendEnable = FALSE;

    D3D12_RENDER_TARGET_BLEND_DESC rtBlend = {};

    rtBlend.BlendEnable = FALSE;
    rtBlend.LogicOpEnable = FALSE;

    rtBlend.SrcBlend = D3D12_BLEND_ONE;
    rtBlend.DestBlend = D3D12_BLEND_ZERO;
    rtBlend.BlendOp = D3D12_BLEND_OP_ADD;

    rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
    rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
    rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;

    rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;

    rtBlend.RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;

    blendDesc.RenderTarget[0] = rtBlend;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = m_rootSignature.Get();

    psoDesc.VS =
    {
        m_vertexShader->GetBufferPointer(),
        m_vertexShader->GetBufferSize()
    };

    psoDesc.PS =
    {
        m_pixelShader->GetBufferPointer(),
        m_pixelShader->GetBufferSize()
    };

    psoDesc.RasterizerState = rasterizerDesc;
    psoDesc.BlendState = blendDesc;

    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.SampleMask = UINT_MAX;

    psoDesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    psoDesc.SampleDesc.Count = 1;

    HRESULT result = m_device->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&m_pipelineState)
    );

    return SUCCEEDED(result);
}

bool Renderer::CreateVertexBuffer()
{
    Vertex vertices[] =
    {
        { { 0.0f, 0.25f, 0.0f }, { 1,0,0,1 } },
        { { 0.25f,-0.25f,0.0f }, { 0,1,0,1 } },
        { {-0.25f,-0.25f,0.0f }, { 0,0,1,1 } },
    };

    const UINT bufferSize = sizeof(vertices);

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = bufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT result = m_device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_vertexBuffer)
    );

    if (FAILED(result))
    {
        return false;
    }

    void* mappedData = nullptr;

    D3D12_RANGE readRange = {};

    m_vertexBuffer->Map(0, &readRange, &mappedData);

    memcpy(mappedData, vertices, sizeof(vertices));

    m_vertexBuffer->Unmap(0, nullptr);

    m_vertexBufferView.BufferLocation =
        m_vertexBuffer->GetGPUVirtualAddress();

    m_vertexBufferView.StrideInBytes = sizeof(Vertex);

    m_vertexBufferView.SizeInBytes = bufferSize;

    return true;
}