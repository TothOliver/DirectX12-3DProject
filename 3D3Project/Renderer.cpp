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

    OutputDebugStringW(L"Renderer initialized.\n");
    return true;
}

void Renderer::Render()
{
    static UINT frameCounter = 0;
    frameCounter++;

    if (frameCounter % 120 == 0)
    {
        //OutputDebugStringW(L"Renderer::Render() is running.\n");
    }
}

void Renderer::Shutdown()
{
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

        DXGI_ADAPTER_DESC1 adapterDescription = {};
        adapter->GetDesc1(&adapterDescription);

        if (adapterDescription.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            adapterIndex++;
            continue;
        }

        m_adapter = adapter;

        wchar_t message[256];
        swprintf_s(message, L"Selected adapter: %s\n", adapterDescription.Description);

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
