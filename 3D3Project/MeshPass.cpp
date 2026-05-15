#include "MeshPass.hpp"

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

bool MeshPass::Initialize(ID3D12Device* device, DXGI_FORMAT renderTargetFormat, UINT width, UINT height)
{
    if (!CreateDeviceDependantResources(device, renderTargetFormat, width, height)) { return false; }

    OutputDebugStringW(L"TrianglePass initialized.\n");
    return true;
}

void MeshPass::Draw(ID3D12GraphicsCommandList* commandList)
{
    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());

    commandList->SetPipelineState(m_pipelineState.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    commandList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
}

void MeshPass::Shutdown()
{
    if (m_constantBuffer && m_mappedConstantBuffer)
    {
        m_constantBuffer->Unmap(0, nullptr);
        m_mappedConstantBuffer = nullptr;
    }
}

bool MeshPass::CreateDeviceDependantResources(ID3D12Device* device, DXGI_FORMAT renderTargetFormat, 
    UINT width, UINT height)
{
    if (!CreateRootSignature(device)) { return false; }

    if (!CreateShaders()) { return false; }

    if (!CreatePipelineState(device, renderTargetFormat)) { return false; }

    if (!CreateVertexBuffer(device)) { return false; }

    if (!CreateIndexBuffer(device)) { return false; }

    if (!CreateConstantBuffer(device, width, height)) { return false; }

    m_viewport.TopLeftX = 0;
    m_viewport.TopLeftY = 0;
    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(width);
    m_scissorRect.bottom = static_cast<LONG>(height);

    return true;
}

bool MeshPass::CreateRootSignature(ID3D12Device* device)
{
    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 1;
    desc.pParameters = rootParameters;
    desc.NumStaticSamplers = 0;
    desc.pStaticSamplers = nullptr;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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
        OutputDebugStringW(L"Failed to serialize root signature.\n");
        return false;
    }

    result = device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    return SUCCEEDED(result);
}

bool MeshPass::CreateShaders()
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

bool MeshPass::CreatePipelineState(ID3D12Device* device, DXGI_FORMAT renderTargetFormat)
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
    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK; //D3D12_CULL_MODE_NONE test
    rasterizerDesc.FrontCounterClockwise = FALSE;

    rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

    rasterizerDesc.DepthClipEnable = TRUE;
    rasterizerDesc.MultisampleEnable = FALSE;
    rasterizerDesc.AntialiasedLineEnable = FALSE;
    rasterizerDesc.ForcedSampleCount = 0;
    rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

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

    rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

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
    psoDesc.RTVFormats[0] = renderTargetFormat;

    psoDesc.SampleDesc.Count = 1;

    HRESULT result = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));

    return SUCCEEDED(result);
}

bool MeshPass::CreateVertexBuffer(ID3D12Device* device)
{
    Vertex vertices[] =
    {
        //Front face
        { { -0.5f, -0.5f, -0.5f }, { 1, 0, 0, 1 } },
        { { -0.5f,  0.5f, -0.5f }, { 1, 0, 0, 1 } },
        { {  0.5f,  0.5f, -0.5f }, { 1, 0, 0, 1 } },
        { {  0.5f, -0.5f, -0.5f }, { 1, 0, 0, 1 } },

        //Back face
        { {  0.5f, -0.5f,  0.5f }, { 0, 1, 0, 1 } },
        { {  0.5f,  0.5f,  0.5f }, { 0, 1, 0, 1 } },
        { { -0.5f,  0.5f,  0.5f }, { 0, 1, 0, 1 } },
        { { -0.5f, -0.5f,  0.5f }, { 0, 1, 0, 1 } },

        //Left face
        { { -0.5f, -0.5f,  0.5f }, { 0, 0, 1, 1 } },
        { { -0.5f,  0.5f,  0.5f }, { 0, 0, 1, 1 } },
        { { -0.5f,  0.5f, -0.5f }, { 0, 0, 1, 1 } },
        { { -0.5f, -0.5f, -0.5f }, { 0, 0, 1, 1 } },

        //Right face
        { {  0.5f, -0.5f, -0.5f }, { 1, 1, 0, 1 } },
        { {  0.5f,  0.5f, -0.5f }, { 1, 1, 0, 1 } },
        { {  0.5f,  0.5f,  0.5f }, { 1, 1, 0, 1 } },
        { {  0.5f, -0.5f,  0.5f }, { 1, 1, 0, 1 } },

        //Top face
        { { -0.5f,  0.5f, -0.5f }, { 1, 0, 1, 1 } },
        { { -0.5f,  0.5f,  0.5f }, { 1, 0, 1, 1 } },
        { {  0.5f,  0.5f,  0.5f }, { 1, 0, 1, 1 } },
        { {  0.5f,  0.5f, -0.5f }, { 1, 0, 1, 1 } },

        //Bottom face
        { { -0.5f, -0.5f,  0.5f }, { 0, 1, 1, 1 } },
        { { -0.5f, -0.5f, -0.5f }, { 0, 1, 1, 1 } },
        { {  0.5f, -0.5f, -0.5f }, { 0, 1, 1, 1 } },
        { {  0.5f, -0.5f,  0.5f }, { 0, 1, 1, 1 } },
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

    HRESULT result = device->CreateCommittedResource(
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

    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();

    m_vertexBufferView.StrideInBytes = sizeof(Vertex);

    m_vertexBufferView.SizeInBytes = bufferSize;

    return true;
}

bool MeshPass::CreateIndexBuffer(ID3D12Device* device)
{
    uint16_t indices[] =
    {
        //Front face
        0, 1, 2,
        0, 2, 3,

        //Back face
        4, 5, 6,
        4, 6, 7,

        //Left face
        8, 9, 10,
        8, 10, 11,

        //Right face
        12, 13, 14,
        12, 14, 15,

        //Top face
        16, 17, 18,
        16, 18, 19,

        //Bottom face
        20, 21, 22,
        20, 22, 23,
    };

    m_indexCount = _countof(indices);

    const UINT bufferSize = sizeof(indices);

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

    HRESULT result = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, 
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer));

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to create index buffer.\n");
        return false;
    }

    void* mappedData = nullptr;

    D3D12_RANGE readRange = {};
    result = m_indexBuffer->Map(0, &readRange, &mappedData);

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to create map index buffer.\n");
        return false;
    }

    memcpy(mappedData, indices, sizeof(indices));

    m_indexBuffer->Unmap(0, nullptr);

    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = bufferSize;

    OutputDebugStringW(L"Index buffer created.\n");
    return true;
}

bool MeshPass::CreateConstantBuffer(ID3D12Device* device, UINT width, UINT height)
{
    const UINT constantBufferSize = (sizeof(TransformConstantBuffer) + 255) & ~255;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = constantBufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT result = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, 
        &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer));

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to create constant buffer.\n");
        return false;
    }

    D3D12_RANGE readRange = {};
    result = m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mappedConstantBuffer));

    if (FAILED(result))
    {
        OutputDebugStringW(L"Failed to map constant buffer.\n");
        return false;
    }

    UpdateConstantBuffer(width, height);

    OutputDebugStringW(L"Constant buffer created.\n");
    return true;
}

void MeshPass::UpdateConstantBuffer(UINT width, UINT height)
{
    using namespace DirectX;

    XMMATRIX world = XMMatrixIdentity();

    XMVECTOR eyePosition = XMVectorSet(0.0f, 0.0f, -3.0f, 1.0f);
    XMVECTOR focusPoint = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
    XMVECTOR upDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    XMMATRIX projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(60.0f), aspectRatio, 0.1f, 100.0f);

    XMMATRIX worldViewProjection = world * view * projection;

    XMStoreFloat4x4(&m_mappedConstantBuffer->WorldViewProjection, XMMatrixTranspose(worldViewProjection));

}
